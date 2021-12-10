#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include "articulated_body.h"
#include "bvh.h"
#include "filesystem_helper.hpp"
#include "posture_graph.h"

#include "parallel_thread_helper.hpp"

class CThreadTrim : public CThread_W32
{
public:
	CThreadTrim()
		: m_trimmed(false)
	{
	}

	~CThreadTrim()
	{
	}

	void Initialize(const std::vector<const char*>& nameSubRM)
	{
		m_nameSubRM = nameSubRM;
	}

	void trim_pre_main(std::string& path_src, std::string& path_dst)
	{
		m_path_src = std::move(path_src);
		m_path_dst = std::move(path_dst);
		Execute_main();
	}

	bool trim_post_main(std::string& path_src, std::string& path_dst, bool& converted)
	{
		assert(m_path_src.empty() == m_path_dst.empty());
		if (m_path_src.empty() || m_path_dst.empty())
			return false;
		path_src = std::move(m_path_src);
		path_dst = std::move(m_path_dst);
		converted = m_trimmed;
		return true;
	}

private:
	virtual void Run_worker()
	{
		m_trimmed = trim(m_path_src.c_str(), m_path_dst.c_str(), m_nameSubRM.data(), (int)m_nameSubRM.size());
		// std::cout << "Trimming " << path_src << " to " << path_dst << ":";
		// bool trimmed = trim(path_src, path_dst, name_subtrees_rm.data(), (int)name_subtrees_rm.size());
		// if (trimmed)
		// 	std::cout << "successful!" << std::endl;
		// else
		// 	std::cout << "failed!!!" << std::endl;
	}
private:
	std::string m_path_src;
	std::string m_path_dst;
	volatile bool m_trimmed;
	std::vector<const char*> m_nameSubRM;
};


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (argc < 6)
	{
		std::cout << "Usage:\thtr_bvh_trim <DIR_SRC> <DIR_DST> <FILE_NAME> <N_Threads> <SubTreeRM>+ \t\t\t\t\t// to trim the articulated body tree file <FILE_NAME> stored in <DIR_SRC>, the trimming result is stored in <DIR_DST>" << std::endl;
	}
	else
	{
		const int i_base_subtree = 5;
		int n_subtrees_rm = argc-i_base_subtree;
		std::vector<const char*> name_subtrees_rm(n_subtrees_rm);
		for (int i_subtree = 0; i_subtree < n_subtrees_rm; i_subtree ++)
			name_subtrees_rm[i_subtree] = argv[i_subtree + i_base_subtree];

		const char* dir_src = argv[1];
		const char* dir_dst = argv[2];
		std::string file_name = argv[3];
		const int n_threads = atoi(argv[4]);
		auto tick_start = ::GetTickCount64();
		int n_failed_case = 0;
		CThreadPool_W32<CThreadTrim> pool;
		pool.Initialize_main(n_threads
							, [&name_subtrees_rm](CThreadTrim* thread)
								{
									thread->Initialize(name_subtrees_rm);
								});

		auto OnTrimFile = [&name_subtrees_rm = std::as_const(name_subtrees_rm), &pool, &n_failed_case](const char* path_src, const char* path_dst) -> bool
			{
				CThreadTrim* worker_i = pool.WaitForAReadyThread_main(INFINITE);
				bool ret_out = false;
				std::string path_src_out;
				std::string path_dst_out;
				if (worker_i->trim_post_main( path_src_out
											, path_dst_out
											, ret_out))
				{
					const char* res[] = { "failed", "successful" };
					int i_res = (ret_out ? 1 : 0);
					printf("Trimming from, %s, to, %s: %s\n"
						, path_src_out.c_str(), path_dst_out.c_str(), res[i_res]);
					if (!ret_out)
						n_failed_case ++;
				}
				std::string path_src_in(path_src);
				std::string path_dst_in(path_dst);
				worker_i->trim_pre_main(path_src_in, path_dst_in);

				// std::cout << "Trimming " << path_src << " to " << path_dst << ":";
				// bool trimmed = trim(path_src, path_dst, name_subtrees_rm.data(), (int)name_subtrees_rm.size());
				// if (trimmed)
				// 	std::cout << "successful!" << std::endl;
				// else
				// 	std::cout << "failed!!!" << std::endl;
				return true;
			};

		try
		{
			CopyDirTree_file(dir_src, dir_dst, OnTrimFile, file_name);
		}
		catch(const std::string& exp)
		{
			std::cout << exp;
		}

		std::vector<CThreadTrim*>& all_threads = pool.WaitForAllReadyThreads_main();

		for (auto worker_i : all_threads)
		{
			bool ret_out = false;
			std::string path_src_out;
			std::string path_dst_out;
			if (worker_i->trim_post_main( path_src_out
										, path_dst_out
										, ret_out))
			{
				const char* res[] = { "failed", "successful" };
				int i_res = (ret_out ? 1 : 0);
				printf("*Trimming from, %s, to, %s: %s\n"
					, path_src_out.c_str(), path_dst_out.c_str(), res[i_res]);
				if (!ret_out)
					n_failed_case ++;
			}
		}

		auto tick_cnt = ::GetTickCount64() - tick_start;
		printf("************TOTAL TIME: %.2f seconds, with %d failures*************\n"
				, (double)tick_cnt/(double)1000
				, n_failed_case);

	}



	return 0;
}
