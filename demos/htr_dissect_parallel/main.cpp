#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include <filesystem>
#include "posture_graph.h"
#include "filesystem_helper.hpp"

#include "parallel_thread_helper.hpp"

class CThreadHTRDisect : public CThread_W32
{
public:
	CThreadHTRDisect()
		: m_disected(false)
	{
	}

	~CThreadHTRDisect()
	{
	}

	void Initialize(const std::string& path_xml)
	{
		c_path_xml = path_xml;
	}

	void disect_pre_main(std::string& path_src, std::string& path_dst)
	{
		m_path_src = std::move(path_src);
		m_path_dst = std::move(path_dst);
		Execute_main();
	}

	bool disect_post_main(std::string& path_src, std::string& path_dst, bool& disected)
	{
		assert(m_path_src.empty() == m_path_dst.empty());
		if (m_path_src.empty() || m_path_dst.empty())
			return false;
		path_src = std::move(m_path_src);
		path_dst = std::move(m_path_dst);
		disected = m_disected;
		return true;
	}

private:
	virtual void Run_worker()
	{
		auto outDir = fs::path(m_path_dst).replace_extension();
		fs::create_directory(outDir);
		m_disected = dissect(c_path_xml.c_str(), m_path_src.c_str(), outDir.u8string().c_str());
	}
private:
	std::string c_path_xml;
	std::string m_path_src;
	std::string m_path_dst;
	volatile bool m_disected;
};

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (5 != argc)
	{
		std::cout << "Usage:\thtr_dissect <PG.XML> <DIR_SRC> <DIR_DST> <N_Threads>" << std::endl;
		return -1;
	}
	else
	{
		typedef bool (*OnFile)(const char* path_src, const char* path_dst);
		std::string path_xml = argv[1];
		const char* dir_src = argv[2];
		const char* dir_dst = argv[3];
		const int n_threads = atoi(argv[4]);
		auto tick_start = ::GetTickCount64();
		int n_failed_case = 0;
		CThreadPool_W32<CThreadHTRDisect> pool;
		pool.Initialize_main(n_threads,
							[&path_xml](CThreadHTRDisect* thread)
							{
								thread->Initialize(path_xml);
							});

		auto onhtr = [&n_failed_case, &pool] (const char* path_src, const char* path_dst) -> bool
			{
				CThreadHTRDisect* worker_i = pool.WaitForAReadyThread_main(INFINITE);
				bool ret_out = false;
				std::string path_src_out;
				std::string path_dst_out;
				if (worker_i->disect_post_main( path_src_out
											, path_dst_out
											, ret_out))
				{
					const char* res[] = { "failed", "successful" };
					int i_res = (ret_out ? 1 : 0);
					printf("dissect from, %s, to, %s: %s\n"
						, path_src_out.c_str(), path_dst_out.c_str(), res[i_res]);
					if (!ret_out)
						n_failed_case ++;
				}
				std::string path_src_in(path_src);
				std::string path_dst_in(path_dst);
				worker_i->disect_pre_main(path_src_in, path_dst_in);

				// std::cout << "onbvh: " <<  path_src << "\t" << path_dst_htr.generic_u8string() << std::endl;
				return true;
			};

		try
		{
			CopyDirTree(dir_src, dir_dst, onhtr, ".htr");
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}

		std::vector<CThreadHTRDisect*>& all_threads = pool.WaitForAllReadyThreads_main();

		for (auto worker_i : all_threads)
		{
			bool ret_out = false;
			std::string path_src_out;
			std::string path_dst_out;
			if (worker_i->disect_post_main( path_src_out
										, path_dst_out
										, ret_out))
			{
				const char* res[] = { "failed", "successful" };
				int i_res = (ret_out ? 1 : 0);
				printf("*Disect from, %s, to, %s: %s\n"
					, path_src_out.c_str(), path_dst_out.c_str(), res[i_res]);
				if (!ret_out)
					n_failed_case ++;
			}
		}

		auto tick = ::GetTickCount64() - tick_start;
		float tick_sec = tick / 1000.0f;
		printf("Converting %s to %s takes %.2f seconds with %d failed cases\n", dir_src, dir_dst, tick_sec, n_failed_case);
	}

	return 0;
}
