#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include "filesystem_helper.hpp"
#include "posture_graph.h"
#include "parallel_thread_helper.hpp"


class Bucket
{
private:
	std::vector<HPG> m_pgs;
};

class BucketStreamIn : public Bucket
{

};

class BucketStatic : public Bucket
{

};

class CMergeThread : public CThread_W32
{
public:
	struct MergeRes
	{
		HPG src_0;
		HPG src_1;
		HPG res;
		bool ok;
	};
	CMergeThread()
	{
	}
	void Initialize(const char* interests_conf, Real eps_err)
	{
		m_interest_conf = interests_conf;
		m_epsErr = eps_err;
	}
	virtual ~CMergeThread()
	{
	}

	void MergeStart_main(HPG src_0, HPG src_1)
	{
		m_res.src_0 = src_0;
		m_res.src_1 = src_1;
		m_res.res = H_INVALID;
		m_res.ok = false;
		Execute_main();
	}

	MergeRes MergeEnd_main()
	{
		return m_res;
	}
private:
	virtual void Run_worker()
	{
		m_res.ok = posture_graph_merge(m_res.src_0, m_res.src_1, m_interest_conf.c_str(), m_epsErr);
	}

private:
	volatile MergeRes m_res;
	volatile std::string m_interest_conf;
	volatile Real m_epsErr;
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
		const int n_threads = atoi(argv[6]);

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

		//	for (auto path: dirs_src)
		//		std::cout << path << std::endl;

		const int N_BUCKET = 5;
		Bucket bucket(N_BUCKET, dirs_src);

		CThreadPool_W32<CMergeThread> pool;
		pool.Initialize_main(n_threads);

		std::vector<CMergeThread*>& threads = pool.WaitForAllReadyThreads_main();
		for (auto it_thread = threads.begin()
			; threads.end() != it_thread
				&& bucket.Size() > 1
			; it_thread ++ )
		{
			std::pair<HPG, HPG> merge_src = bucket.Pop_pair();
			(*it_thread)->Initialize(path_interests_conf, eps_err);
			(*it_thread)->MergeStart_main(merge_src.first, merge_src.second);
			bucket.PumpIn();
			bucket.PumpIn();
		}

		while (bucket.Size() > 1)
		{
			CMergeThread* thread = pool.WaitForAReadyThread_main();
			std::pair<HPG, HPG> merge_src = bucket.Pop_pair();
			auto merge_res = thread->MergeEnd_main();
			if (merge_res.ok)
			{
				bucket.Push(merge_res.res);
				bucket.PumpIn();
				Bucket::PumpOut(merge_res.src_0);
				Bucket::PumpOut(merge_res.src_1);
			}
			else
			{
				bucket.Push(merge_res.src_0);
				bucket.Push(merge_res.src_1);
			}
			thread->MergeStart_main(merge_src.first, merge_src.second);
		}

		threads = pool.WaitForAllReadyThreads_main();
		BucketStatic bucket_final(bucket.Size() + 2*n_threads);
		for (int i_ele = 0; i_ele < bucket.Size(); i_ele ++)
			bucket_final.Push(bucket[i_ele]);
		for (auto thread : threads)
		{
			auto merge_res = thread->MergeEnd_main();
			if (merge_res.ok)
			{
				bucket_final.Push(merge_res.res);
				Bucket::PumpOut(merge_res.src_0);
				Bucket::PumpOut(merge_res.src_1);
			}
			else
			{
				bucket_final.Push(merge_res.src_0);
				bucket_final.Push(merge_res.src_1);
			}
		}

		int N_rounds = bucket_final.Size();
		int i_round = 0;
		while (bucket_final.Size() > 1
			&& i_round < N_rounds)
		{
			std::pair<HPG, HPG> merge_src = bucket_final.Pop_pair();
			HPG res = posture_graph_merge(merge_src.first, merge_src.second, path_interests_conf, eps_err);
			if (VALID_HANDLE(res))
			{
				bucket_final.Push(res);
				Bucket::PumpOut(merge_src.first);
				Bucket::PumpOut(merge_src.second);
			}
			else
			{
				bucket_final.Push(merge_src.first);
				bucket_final.Push(merge_src.second);
				if (2 == bucket_final.Size())
					break;
			}
			i_round ++;
		}

		std::string dir_dst_str(dir_dst);
		int i_res = 0;
		do
		{
			std::error_code ec;
			fs::create_directory(fs::path(dir_dst_str), ec);
			save_pg(bucket_final[i_res], dir_dst_str.c_str());
			Bucket::PumpOut(bucket_final[i_res]);
			dir_dst_str += "X"; // to create a different folder for another PG
		} while (i_res < bucket_final.Size());

		auto tick_cnt = ::GetTickCount64() - tick_start;
		printf("************TOTAL TIME: %.2f seconds, merged into %d PGs*************\n", (double)tick_cnt/(double)1000, i_res);

	}

	return 0;
}
