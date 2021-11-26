#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include "filesystem_helper.hpp"
#include "posture_graph.h"
#include "parallel_thread_helper.hpp"

class CThreadPostureGraphGen : public CThread_W32
{
public:
	CThreadPostureGraphGen()
		: m_generated(false)
		, m_nThetaRaw(0)
		, m_nThetaPG(0)
	{
	}

	~CThreadPostureGraphGen()
	{
	}

	void posture_graph_gen_pre_main(std::string& path_interests_conf, std::string& path_src, std::string& dir_dst, Real eps_err)
	{
		m_path_interests_conf = std::move(path_interests_conf);
		m_path_src = std::move(path_src);
		m_dir_dst = std::move(dir_dst);
		m_eps_err = eps_err;
		c_dir_dst = m_dir_dst.c_str();
		c_path_interests_conf = m_path_interests_conf.c_str();
		c_path_src = m_path_src.c_str();
		Execute_main();
	}

	bool posture_graph_gen_post_main(std::string& path_src, std::string& dir_dst, Real& eps_err, unsigned int & dur_milli, bool& generated, int& n_theta_raw, int& n_theta_pg)
	{
		assert(m_path_src.empty() == m_dir_dst.empty());
		if (m_path_src.empty() || m_dir_dst.empty())
			return false;
		dur_milli = Dur_main();
		path_src = std::move(m_path_src);
		dir_dst = std::move(m_dir_dst);
		eps_err = m_eps_err;
		generated = m_generated;
		n_theta_raw = m_nThetaRaw;
		n_theta_pg = m_nThetaPG;
		return true;
	}

	virtual void Run_worker()
	{
		m_generated = posture_graph_gen(c_path_interests_conf
									, c_path_src
									, c_dir_dst
									, m_eps_err
									, NULL
									, &m_nThetaRaw
									, &m_nThetaPG);
	}
private:
	std::string m_path_interests_conf;
	std::string m_path_src;
	std::string m_dir_dst;
	const char* volatile c_path_interests_conf;
	const char* volatile c_path_src;
	const char* volatile c_dir_dst;
	volatile Real m_eps_err;
	volatile bool m_generated;
	int m_nThetaRaw;
	int m_nThetaPG;
};


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (6 != argc)
	{
		std::cout << "Usage:\tposture_graph_gen <INTERESTS_XML> <BVH_DIR_SRC> <BVH_DIR_DST> <Epsilon> <N_threads>" << std::endl;
		return -1;
	}
	else
	{
		const char* path_interests_conf = argv[1];
		const char* dir_src = argv[2];
		const char* dir_dst = argv[3];
		Real eps_err = (Real)atof(argv[4]);
		const int n_threads = atoi(argv[5]);

		auto tick_start = ::GetTickCount64();

		CThreadPool_W32<CThreadPostureGraphGen> pool;
		pool.Initialize_main(n_threads, [](CThreadPostureGraphGen*){});
		int n_theta_raw = 0;
		int n_theta_pgs = 0;
		auto onhtr = [path_interests_conf, eps_err, &pool, &n_theta_raw, &n_theta_pgs] (const char* path_src, const char* path_dst) -> bool
			{
				CThreadPostureGraphGen* posture_graph_gen_worker_i = pool.WaitForAReadyThread_main(INFINITE);
				unsigned int dur_milli_out = 0;
				bool ret_out = false;
				std::string path_src_out;
				std::string dir_dst_out;
				Real eps_err_out;
				int n_theta_raw_i = 0;
				int n_theta_pgs_i = 0;
				if (posture_graph_gen_worker_i->posture_graph_gen_post_main( path_src_out
																			, dir_dst_out
																			, eps_err_out
																			, dur_milli_out
																			, ret_out
																			, n_theta_raw_i
																			, n_theta_pgs_i))
				{
					n_theta_raw += n_theta_raw_i;
					n_theta_pgs += n_theta_pgs_i;
					const char* res[] = { "failed", "successful" };
					int i_res = (ret_out ? 1 : 0);
					printf("Building Posture-Graph from, %s, to, %s, takes %.2f seconds:, reduce posture from, %d, to, %d, %s\n"
						, path_src_out.c_str(), dir_dst_out.c_str(), (double)dur_milli_out/(double)1000, n_theta_raw_i, n_theta_pgs_i, res[i_res]);
				}
				std::string path_conf_worker_in(path_interests_conf);
				std::string path_src_in(path_src);
				std::string dir_dst_in = fs::path(path_dst).parent_path().u8string();
				posture_graph_gen_worker_i->posture_graph_gen_pre_main(path_conf_worker_in, path_src_in, dir_dst_in, eps_err);
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

		std::vector<CThreadPostureGraphGen*>& all_threads = pool.WaitForAllReadyThreads_main();

		for (auto thread_i : all_threads)
		{
			unsigned int dur_milli_out = 0;
			bool ret_out = false;
			std::string path_src_out;
			std::string dir_dst_out;
			Real eps_err_out;
			int n_theta_raw_i = 0;
			int n_theta_pg_i = 0;
			if (thread_i->posture_graph_gen_post_main( path_src_out
													, dir_dst_out
													, eps_err_out
													, dur_milli_out
													, ret_out
													, n_theta_raw_i
													, n_theta_pg_i))
			{
				n_theta_raw += n_theta_raw_i;
				n_theta_pgs += n_theta_pg_i;
				const char* res[] = { "failed", "successful" };
				int i_res = (ret_out ? 1 : 0);
					printf("Building Posture-Graph from, %s, to, %s, takes %.2f seconds:, reduce posture from, %d, to, %d, %s\n"
						, path_src_out.c_str(), dir_dst_out.c_str(), (double)dur_milli_out/(double)1000, n_theta_raw_i, n_theta_pg_i, res[i_res]);
			}
		}

		auto tick_cnt = ::GetTickCount64() - tick_start;
		printf("************TOTAL TIME: %.2f seconds, build PG of %d postures from %d frames of record*************\n"
			, (double)tick_cnt/(double)1000
			, n_theta_pgs
			, n_theta_raw);

	}



	return 0;
}
