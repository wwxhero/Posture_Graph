#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include <filesystem>
#include "articulated_body.h"
#include "bvh.h"
#include "filesystem_helper.hpp"

#include "parallel_thread_helper.hpp"

class CThreadTReset : public CThread_W32
{
public:
	CThreadTReset()
		: m_pose_id(1)
		, m_reset(false)
	{
	}

	~CThreadTReset()
	{
	}

	void posture_reset_pre_main(std::string& path_src, std::string& path_dst)
	{
		m_path_src = std::move(path_src);
		m_path_dst = std::move(path_dst);
		Execute_main();
	}

	bool posture_reset_post_main(std::string& path_src, std::string& path_dst, bool& reset)
	{
		assert(m_path_src.empty() == m_path_dst.empty());
		if (m_path_src.empty() || m_path_dst.empty())
			return false;
		path_src = std::move(m_path_src);
		path_dst = std::move(m_path_dst);
		reset = m_reset;
		return true;
	}

	virtual void Run_worker()
	{
		m_reset = ResetRestPose(m_path_src.c_str()
								, m_pose_id
								, m_path_dst.c_str()
								, 1.0);
	}
private:
	std::string m_path_src;
	std::string m_path_dst;
	const int m_pose_id;
	volatile bool m_reset;
};

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (4 != argc)
	{
		std::cout << "Usage:\tbvh_posture_rest <BVH_DIR_SRC> <BVH_DIR_DST> <N_Threads>" << std::endl;
		return -1;
	}
	else
	{
		const char* dir_src = argv[1];
		const char* dir_dst = argv[2];
		const int n_threads = atoi(argv[3]);
		auto tick_start = ::GetTickCount64();
		int n_failed_case = 0;
		CThreadPool_W32<CThreadTReset> pool;
		pool.Initialize_main(n_threads, [](CThreadTReset*){});

		auto onbvh = [&n_failed_case, &pool] (const char* path_src, const char* path_dst) -> bool
			{
				CThreadTReset* posture_reset_worker_i = pool.WaitForAReadyThread_main(INFINITE);
				bool ret_out = false;
				std::string path_src_out;
				std::string path_dst_out;
				if (posture_reset_worker_i->posture_reset_post_main( path_src_out
																, path_dst_out
																, ret_out))
				{
					const char* res[] = { "failed", "successful" };
					int i_res = (ret_out ? 1 : 0);
					printf("Reset postures from, %s, to, %s: %s\n"
						, path_src_out.c_str(), path_dst_out.c_str(), res[i_res]);
					if (!ret_out)
						n_failed_case ++;
				}
				std::string path_src_in(path_src);
				std::string path_dst_in(path_dst);
				posture_reset_worker_i->posture_reset_pre_main(path_src_in, path_dst_in);
				return true;
			};
		try
		{
			// std::cerr << "Usage:\tbvh_posture_rest <BVH_DIR_SRC> <BVH_DIR_DST> <POSE_ID>" << std::endl;
			CopyDirTree(dir_src, dir_dst, onbvh, ".bvh");
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}

		std::vector<CThreadTReset*>& all_threads = pool.WaitForAllReadyThreads_main();

		for (auto posture_reset_worker_i : all_threads)
		{
			bool ret_out = false;
			std::string path_src_out;
			std::string path_dst_out;
			if (posture_reset_worker_i->posture_reset_post_main( path_src_out
															, path_dst_out
															, ret_out))
			{
				const char* res[] = { "failed", "successful" };
				int i_res = (ret_out ? 1 : 0);
				printf("*Reset postures from, %s, to, %s: %s\n"
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
