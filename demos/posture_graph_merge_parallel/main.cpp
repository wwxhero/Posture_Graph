#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include <queue>
#include "filesystem_helper.hpp"
#include "posture_graph.h"
#include "parallel_thread_helper.hpp"

#define MIN_N_THETA 10

class Merge
{
public:
	Merge(Real a_eps, const std::string& dir_pg, const std::string& pg_name) // throw
		: src_min(nullptr)
		, src_max(nullptr)
		, hpg(H_INVALID)
		, eps(a_eps)
	{
		hpg = posture_graph_load(dir_pg.c_str(), pg_name.c_str());
		cov = (Real)N_Theta(hpg);
	}

	Merge(std::shared_ptr<Merge> src_0, std::shared_ptr<Merge> src_1)
		: hpg(H_INVALID)
	{
		if (src_0->cov < src_1->cov)
		{
			src_min = src_0;
			src_max = src_1;
		}
		else
		{
			src_min = src_1;
			src_max = src_0;
		}
		eps = max(src_0->eps, src_1->eps);
		cov = src_0->cov + src_1->cov;
	}

	~Merge()
	{
		if (VALID_HANDLE(hpg))
			posture_graph_release(hpg);
		hpg = H_INVALID;
	}

	void Done(Real a_eps)
	{
		cov = (Real)N_Theta(hpg);
		eps = a_eps;
		src_min = nullptr;
		src_max = nullptr;
	}

	void Abort(Real decay, Real decay_inv)
	{
		src_min->cov = decay * src_min->cov;
		src_min->eps = decay_inv * src_min->eps;
	}

	void Exe(const char* conf_path)
	{
		hpg = posture_graph_merge(src_min->hpg, src_max->hpg, conf_path, eps);
	}

	std::shared_ptr<Merge> src_min;
	std::shared_ptr<Merge> src_max;
	HPG hpg;
	Real cov;
	Real eps;
};

class LessPGCov
{
public:
	explicit LessPGCov()
	{
	}

	bool operator()(std::shared_ptr<Merge> left, std::shared_ptr<Merge> right) const
	{
		return left->cov < right->cov;
	}

};


class Bucket
{
public:
	Bucket(int bucketSize, Real eps_dft, std::list<std::string>& paths, const char* pg_name)
		: c_epsErr(eps_dft)
		, m_pgDirs(std::move(paths))
		, m_itDir(m_pgDirs.begin())
		, c_strPGName(pg_name)
		, c_decay((Real)0.9)
		, c_decayinv((Real)1.0/ (Real)0.9)
		, m_nPumped(0)
		, m_nTheta(0)
		, m_nFailure(0)
	{
		for (int n_ele = 0; n_ele < bucketSize; n_ele ++)
		{
			std::shared_ptr<Merge> ele = PumpIn();
			Push(ele);
		}
	}

	void Push(std::shared_ptr<Merge> toMerge)
	{
		if (toMerge && toMerge->cov > MIN_N_THETA)
		{
			m_mergingQ.push(toMerge);
		}
	}

	std::shared_ptr<Merge> PumpIn()
	{
		if (m_itDir != m_pgDirs.end())
		{
			try
			{
				auto ret = std::make_shared<Merge>(c_epsErr, *m_itDir, c_strPGName);
				std::cout << "Loading " << *m_itDir << " results " << ret->cov << " postures" << std::endl;
				m_itDir ++;
				m_nPumped ++;
				m_nTheta += (int)(ret->cov);
				return ret;
			}
			catch(std::exception& e)
			{
				std::cout << e.what();
				return std::shared_ptr<Merge>();
			}
		}
		else
			return std::shared_ptr<Merge>();
	}

	void PushMergeRes(std::shared_ptr<Merge> merged)
	{
		if (VALID_HANDLE(merged->hpg))
		{
			std::cout << "Merging between posture graphs of "
						<< N_Theta(merged->src_min->hpg)
						<< " postures and "
						<< N_Theta(merged->src_max->hpg)
						<< " postures results "
						<< N_Theta(merged->hpg) 
						<< " postures!" << std::endl;
			merged->Done(c_epsErr);
			Push(merged);
			Push(PumpIn());
		}
		else
		{
			std::cout << "Merging between posture graphs of "
						<< N_Theta(merged->src_min->hpg)
						<< " postures and "
						<< N_Theta(merged->src_max->hpg)
						<< " failed!!! " << std::endl;
			merged->Abort(c_decay, c_decayinv);
			Push(merged->src_min);
			Push(merged->src_max);
			merged = nullptr;
			m_nFailure ++;
		}
	}

	std::size_t Size() const
	{
		return m_mergingQ.size();
	}

	std::pair<std::shared_ptr<Merge>, std::shared_ptr<Merge>> Pop_pair()
	{
		assert(m_mergingQ.size()>1);
		auto p_0 = m_mergingQ.top(); m_mergingQ.pop();
		auto p_1 = m_mergingQ.top(); m_mergingQ.pop();
		return std::make_pair(p_0, p_1);
	}

	std::shared_ptr<Merge> Pop()
	{
		auto p = m_mergingQ.top();
		m_mergingQ.pop();
		return p;
	}

private:
	std::priority_queue<std::shared_ptr<Merge>, std::vector<std::shared_ptr<Merge>>, LessPGCov> m_mergingQ;
	const Real c_epsErr;
	std::list<std::string> m_pgDirs;
	std::list<std::string>::iterator m_itDir;
	std::string c_strPGName;
	const Real c_decay;
	const Real c_decayinv;
public:
	int m_nPumped;
	int m_nTheta;
	int m_nFailure;
};

class CMergeThread : public CThread_W32
{
public:
	CMergeThread()
		: m_res(nullptr)
	{
	}
	void Initialize(const char* interests_conf)
	{
		m_interest_conf = interests_conf;
	}
	virtual ~CMergeThread()
	{
	}

	void MergeStart_main(std::shared_ptr<Merge> src_0, std::shared_ptr<Merge> src_1)
	{
		m_res = std::make_shared<Merge>(src_0, src_1);
		Execute_main();
	}

	std::shared_ptr<Merge> MergeEnd_main()
	{
		std::shared_ptr<Merge> ret = m_res;
		m_res = nullptr;
		return ret;
	}

private:
	virtual void Run_worker()
	{
		m_res->Exe(m_interest_conf.c_str());
	}

private:
	std::shared_ptr<Merge> m_res;
	std::string m_interest_conf;
};


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (7 != argc)
	{
		std::cout << "Usage:\tposture_graph_merge <INTERESTS_XML> <PG_DIR_SRC> <PG_DIR_DST> <PG_NAME> <Epsilon> <N_threads>" << std::endl;
		return -1;
	}
	else
	{
		const char* path_interests_conf = argv[1];
		const char* dir_src = argv[2];
		const char* dir_dst = argv[3];
		const char* pg_name = argv[4];
		Real eps_err = (Real)atof(argv[5]);
		int n_threads = atoi(argv[6]);

		auto tick_start = ::GetTickCount64();

		std::set<std::string> dirs_src_pg;
		auto OnDirPG = [&dirs_src_pg] (const char* dir)
			{
				dirs_src_pg.insert(dir);
				return true;
			};
		std::list<std::string> dirs_src;
		auto OnDirHTR = [&dirs_src_pg = std::as_const(dirs_src_pg), &dirs_src](const char* dir)
			{
				if (dirs_src_pg.end() != dirs_src_pg.find(dir))
					dirs_src.push_back(dir);
			};
		try
		{
			std::string filter_base(pg_name);
			TraverseDirTree_filter(dir_src, OnDirPG, filter_base + ".pg" );
			TraverseDirTree_filter(dir_src, OnDirHTR, filter_base + ".htr" );
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}

		int half_n_pgs = (((int)dirs_src.size()) >> 1);
		n_threads = min(n_threads, half_n_pgs);
		//	for (auto path: dirs_src)
		//		std::cout << path << std::endl;

		const int N_BUCKET = 5;
		Bucket bucket(N_BUCKET, eps_err, dirs_src, pg_name);

		auto Parallel_N_currency = [&] (int n_threads)
		{
			CThreadPool_W32<CMergeThread> pool;
			pool.Initialize_main(n_threads);

			std::vector<CMergeThread*>& threads = pool.WaitForAllReadyThreads_main();
			for (auto it_thread = threads.begin()
				; threads.end() != it_thread
				; it_thread ++ )
			{
				assert(bucket.Size() > 1);
				auto merge_src = bucket.Pop_pair();
				(*it_thread)->Initialize(path_interests_conf);
				(*it_thread)->MergeStart_main(merge_src.first, merge_src.second);
				bucket.Push(bucket.PumpIn());
				bucket.Push(bucket.PumpIn());
			}

			while (bucket.Size() > 1)
			{
				CMergeThread* thread_i = pool.WaitForAReadyThread_main(INFINITE);
				auto merge_src = bucket.Pop_pair();
				auto merge_res = thread_i->MergeEnd_main();
				bucket.PushMergeRes(merge_res);
				thread_i->MergeStart_main(merge_src.first, merge_src.second);
			}

			threads = pool.WaitForAllReadyThreads_main();
			for (auto thread : threads)
			{
				auto merge_res = thread->MergeEnd_main();
				bucket.PushMergeRes(merge_res);
			}
		};

		auto Parallel_CompleteBucket = [&] ()
		{
			while (bucket.Size() > 1)
			{
				CThreadPool_W32<CMergeThread> pool_finish;
				int n_threads = ((int)bucket.Size() >> 1);
				assert(n_threads > 0);
				pool_finish.Initialize_main(n_threads);
				std::vector<CMergeThread*>& threads = pool_finish.WaitForAllReadyThreads_main();
				for (auto thread_i : threads)
				{
					auto merge_src = bucket.Pop_pair();
					thread_i->Initialize(path_interests_conf);
					thread_i->MergeStart_main(merge_src.first, merge_src.second);
				}
				threads = pool_finish.WaitForAllReadyThreads_main();
				for (auto thread_i : threads)
				{
					auto merge_res = thread_i->MergeEnd_main();
					bucket.PushMergeRes(merge_res);
				}
			}
		};

		if (n_threads > 0)
			Parallel_N_currency(n_threads);
		Parallel_CompleteBucket();

		std::shared_ptr<Merge> res = bucket.Pop();
		save_pg(res->hpg, dir_dst);

		auto tick_cnt = ::GetTickCount64() - tick_start;
		printf("************TOTAL TIME: %.2f seconds: %d files of %d postures in total have been merged into %d postures with %d failures*************\n"
			, (double)tick_cnt/(double)1000
			, bucket.m_nPumped, bucket.m_nTheta
			, N_Theta(res->hpg), bucket.m_nFailure);
	}

	return 0;
}
