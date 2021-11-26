#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include <queue>
#include <sys/stat.h>
#include "filesystem_helper.hpp"
#include "posture_graph.h"
#include "parallel_thread_helper.hpp"

#define MIN_N_THETA 10
#define MAX_N_MATTEMPTS 5

class Merge
{
public:
	Merge(Real a_eps, const std::string& dir_pg, const std::string& pg_name) // throw
		: src_min(nullptr)
		, src_max(nullptr)
		, hpg(H_INVALID)
		, eps(a_eps)
		, n_attempts(0)
	{
		hpg = posture_graph_load(dir_pg.c_str(), pg_name.c_str());
		cov = (Real)N_Theta(hpg);
	}

	Merge(std::shared_ptr<Merge> src_0, std::shared_ptr<Merge> src_1)
		: hpg(H_INVALID)
		, n_attempts(0)
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
		n_attempts = 0;
	}

	void Abort(Real decay, Real decay_inv)
	{
		src_min->cov = decay * src_min->cov;
		src_min->eps = decay_inv * src_min->eps;
		src_min->n_attempts ++;
		src_max->n_attempts ++;
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
	int n_attempts;
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


class TasksQ
{
public:
	TasksQ(int bucketSize, Real eps_dft, std::list<std::string>& paths, const char* pg_name)
		: c_epsErr(eps_dft)
		, m_pgDirs(std::move(paths))
		, m_itDir(m_pgDirs.begin())
		, c_strPGName(pg_name)
		, c_decay((Real)1.0)
		, c_decayinv((Real)1.0)
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
		if (toMerge && toMerge->cov > MIN_N_THETA && toMerge->n_attempts < MAX_N_MATTEMPTS)
		{
			m_mergingQ.push_back(toMerge);
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

	void ProceMergeRes(std::shared_ptr<Merge> merged)
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
			PushFront(merged);
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
		auto p_0 = m_mergingQ.front(); m_mergingQ.pop_front();
		auto p_1 = m_mergingQ.back(); m_mergingQ.pop_back();
		return std::make_pair(p_0, p_1);
	}

	std::shared_ptr<Merge> Pop()
	{
		auto p = m_mergingQ.front();
		m_mergingQ.pop_front();
		return p;
	}

private:
	void PushFront(std::shared_ptr<Merge> merged)
	{
		assert(VALID_HANDLE(merged->hpg));
		m_mergingQ.push_front(merged);
	}

	// back -> front
	// (bad theta file) -> (good theta file)
	std::deque<std::shared_ptr<Merge>> m_mergingQ;
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

		class GreatorFileSize
		{
		public:
			GreatorFileSize(const char* pg_name)
				: m_pgName(pg_name)
			{
				m_pgName += ".htr";
			}
			bool operator()(const std::string &dir_0, const std::string &dir_1) const
			{
				fs::path theta_path_0(dir_0); theta_path_0.append(m_pgName);
				fs::path theta_path_1(dir_1); theta_path_1.append(m_pgName);
				struct stat stat_buf_0;
				int rc_0 = stat(theta_path_0.u8string().c_str(), &stat_buf_0);
				struct stat stat_buf_1;
				int rc_1 = stat(theta_path_1.u8string().c_str(), &stat_buf_1);
				return !(0 == rc_0 && 0 == rc_1)
					|| (stat_buf_0.st_size > stat_buf_1.st_size);
			}
		private:
			std::string m_pgName;
		};

		dirs_src.sort(GreatorFileSize(pg_name));
		//	for (auto path: dirs_src)
		//		std::cout << path << std::endl;

		const int N_BUCKET = 1 + (n_threads << 1);
		TasksQ tasks(N_BUCKET, eps_err, dirs_src, pg_name);

		if (n_threads > 0)
		{
			[&] (int n_threads)	// assign tasks to n_threads until not all threads are assigned
			{
				CThreadPool_W32<CMergeThread> pool;
				pool.Initialize_main(
								n_threads,
								[path_interests_conf](CMergeThread* thread)
									{
										thread->Initialize(path_interests_conf);
									}
								);

				while (tasks.Size() > 1)
				{
					CMergeThread* thread_i = pool.WaitForAReadyThread_main(INFINITE);
					auto merge_src = tasks.Pop_pair();
					auto merge_res = thread_i->MergeEnd_main();
					if (merge_res)
						tasks.ProceMergeRes(merge_res);
					else
					{
						tasks.Push(tasks.PumpIn());
						tasks.Push(tasks.PumpIn());
					}
					thread_i->MergeStart_main(merge_src.first, merge_src.second);
				}

				auto& threads = pool.WaitForAllReadyThreads_main();
				for (auto thread : threads)
				{
					auto merge_res = thread->MergeEnd_main();
					if (merge_res)
						tasks.ProceMergeRes(merge_res);
				}
			}(n_threads);
		}

		[&] () // finish up the merge tasks stored in tasks
		{
			while (tasks.Size() > 1)
			{
				CThreadPool_W32<CMergeThread> pool_finish;
				int n_threads = ((int)tasks.Size() >> 1);
				assert(n_threads > 0);
				pool_finish.Initialize_main(
										n_threads, 
										[path_interests_conf](CMergeThread* thread_i) 
											{
												thread_i->Initialize(path_interests_conf);
											});
				std::vector<CMergeThread*>& threads = pool_finish.WaitForAllReadyThreads_main();
				for (auto thread_i : threads)
				{
					auto merge_src = tasks.Pop_pair();
					thread_i->MergeStart_main(merge_src.first, merge_src.second);
				}
				threads = pool_finish.WaitForAllReadyThreads_main();
				for (auto thread_i : threads)
				{
					auto merge_res = thread_i->MergeEnd_main();
					tasks.ProceMergeRes(merge_res);
				}
			}
		}();

		int n_theta_total = 0;
		if (tasks.Size() > 0)
		{
			std::shared_ptr<Merge> res = tasks.Pop();
			posture_graph_save(res->hpg, dir_dst);
			n_theta_total = N_Theta(res->hpg);
		}

		auto tick_cnt = ::GetTickCount64() - tick_start;
		printf("************TOTAL TIME: %.2f seconds: %d files of %d postures in total have been merged into %d postures with %d failures*************\n"
			, (double)tick_cnt/(double)1000
			, tasks.m_nPumped, tasks.m_nTheta
			, n_theta_total, tasks.m_nFailure);
	}

	return 0;
}
