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

class CThreadBVH2HTR : public CThread_W32
{
public:
	CThreadBVH2HTR()
		: m_converted(false)
	{
	}

	~CThreadBVH2HTR()
	{
	}

	void convert_pre_main(std::string& path_src, std::string& path_dst)
	{
		m_path_src = std::move(path_src);
		m_path_dst = std::move(path_dst);
		Execute_main();
	}

	bool convert_post_main(std::string& path_src, std::string& path_dst, bool& converted)
	{
		assert(m_path_src.empty() == m_path_dst.empty());
		if (m_path_src.empty() || m_path_dst.empty())
			return false;
		path_src = std::move(m_path_src);
		path_dst = std::move(m_path_dst);
		converted = m_converted;
		return true;
	}

private:
	virtual void Run_worker()
	{
		m_converted = convert(m_path_src.c_str(), m_path_dst.c_str(), false);
	}
private:
	std::string m_path_src;
	std::string m_path_dst;
	volatile bool m_converted;
};

void conv(const char* path_src, const char* path_dst)
{
	printf("Converting %s to %s: ", path_src, path_dst);
	bool converted = convert(path_src, path_dst, false);
	const char* res[] = { "failed", "successful" };
	int i_res = (converted ? 1 : 0);
	printf(" %s\n", res[i_res]);
};

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (4 != argc)
	{
		std::cout << "Usage:\tbvh_htr_conv_parallel <BVH_DIR_SRC> <BVH_DIR_DST> <N_Threads>" << std::endl;
		return -1;
	}
	else
	{
		const char* dir_src = argv[1];
		const char* dir_dst = argv[2];
		const int n_threads = atoi(argv[3]);
		auto tick_start = ::GetTickCount64();
		int n_failed_case = 0;
		CThreadPool_W32<CThreadBVH2HTR> pool;
		pool.Initialize_main(n_threads, [](CThreadBVH2HTR*){});
			   
		auto onbvh = [&pool, &n_failed_case] (const char* path_src, const char* path_dst) -> bool
			{
				CThreadBVH2HTR* posture_reset_worker_i = pool.WaitForAReadyThread_main(INFINITE);
				bool ret_out = false;
				std::string path_src_out;
				std::string path_dst_out;
				if (posture_reset_worker_i->convert_post_main( path_src_out
															, path_dst_out
															, ret_out))
				{
					const char* res[] = { "failed", "successful" };
					int i_res = (ret_out ? 1 : 0);
					printf("Converting from, %s, to, %s: %s\n"
						, path_src_out.c_str(), path_dst_out.c_str(), res[i_res]);
					if (!ret_out)
						n_failed_case ++;
				}
				std::string path_src_in(path_src);
				auto path_dst_htr = fs::path(path_dst).replace_extension(fs::path("htr"));
				std::string path_dst_in(path_dst_htr.generic_u8string().c_str());
				posture_reset_worker_i->convert_pre_main(path_src_in, path_dst_in);

				// std::cout << "onbvh: " <<  path_src << "\t" << path_dst_htr.generic_u8string() << std::endl;
				return true;
			};

		try
		{
			CopyDirTree(dir_src, dir_dst, onbvh, ".bvh");
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}

		std::vector<CThreadBVH2HTR*>& all_threads = pool.WaitForAllReadyThreads_main();

		for (auto posture_reset_worker_i : all_threads)
		{
			bool ret_out = false;
			std::string path_src_out;
			std::string path_dst_out;
			if (posture_reset_worker_i->convert_post_main( path_src_out
														, path_dst_out
														, ret_out))
			{
				const char* res[] = { "failed", "successful" };
				int i_res = (ret_out ? 1 : 0);
				printf("*Converting from, %s, to, %s: %s\n"
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
