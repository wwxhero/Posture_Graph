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
#define MAX_N_ATTEMPTS 6

class CMergeThread : public CThread_W32
{
public:
	CMergeThread()
		: m_itSrcDirs(m_pgSrcDirs.begin())
		, m_hpgSrc0(H_INVALID)
		, m_hpgSrc1(H_INVALID)
		, m_hpgRes(H_INVALID)
		, m_epsErr(0)
		, m_nTheta0(0)
		, m_nPGs(0)
		, m_id(-1)
	{
	}

	void Initialize(const std::string& interests_conf,
					const std::string& pg_name,
					std::list<std::string>& pg_list_dirs,
					const std::string& dir_dst,
					Real eps_err,
					int id_pg)
	{
		m_interest_conf = interests_conf;
		m_pgName = pg_name;
		m_pgSrcDirs = std::move(pg_list_dirs);
		m_itSrcDirs = m_pgSrcDirs.begin();
		m_nPGs = (int)m_pgSrcDirs.size();
		m_pgDstDir = dir_dst;
		m_epsErr = eps_err;
		m_hpgSrc0 = LoadNext_main();
		m_id = id_pg;
	}

	virtual ~CMergeThread()
	{
		HPG* pgs[] = {&m_hpgSrc0, &m_hpgSrc1, &m_hpgRes};
		for (auto pg : pgs)
		{
			if (VALID_HANDLE(*pg))
				posture_graph_release(*pg);
			*pg = H_INVALID;
		}
	}

	void MergeStart_main()
	{
		m_hpgSrc1 = LoadNext_main();
		Execute_main();
	}

	bool MergeEnd_main()
	{
		int n_theta_0 = (VALID_HANDLE(m_hpgSrc0)) ? N_Theta(m_hpgSrc0) : 0;
		int n_theta_1 = (VALID_HANDLE(m_hpgSrc1)) ? N_Theta(m_hpgSrc1) : 0;
		if (VALID_HANDLE(m_hpgRes))
		{
			std::cout << m_id << ": Merging of postures ("
						<< n_theta_0
						<< ", "
						<< n_theta_1
						<< ") results "
						<< N_Theta(m_hpgRes) << std::endl;
			posture_graph_release(m_hpgSrc0);
			posture_graph_release(m_hpgSrc1);
			m_hpgSrc0 = m_hpgRes;
			m_hpgSrc1 = H_INVALID;
			m_hpgRes = H_INVALID;
		}
		else
		{
			std::cout << m_id << ": Merging of postures ("
						<< n_theta_0
						<< ", "
						<< n_theta_1
						<< ") failed!!!" << std::endl;

			if (!(n_theta_0 > n_theta_1))
				std::swap(m_hpgSrc0, m_hpgSrc1);
			if (VALID_HANDLE(m_hpgSrc1))
				posture_graph_release(m_hpgSrc1);
			m_hpgSrc1 = H_INVALID;
			m_hpgRes = H_INVALID;
		}

		return !m_pgSrcDirs.empty(); // m_pgSrcDirs.empty() <-> not able to proceed merging
	}

	void MergeComplete_main()
	{
		int n_theta = 0;
		if (VALID_HANDLE(m_hpgSrc0))
		{
			n_theta = N_Theta(m_hpgSrc0);
			posture_graph_save(m_hpgSrc0, m_pgDstDir.c_str());
			posture_graph_release(m_hpgSrc0);
			m_hpgSrc0 = H_INVALID;
		}
		std::cout << m_id << ": Merging "
					<< m_nTheta0
					<< " postures from "
					<< m_nPGs
					<< " PGs results a PG of "
					<< n_theta
					<< " postures!!!" << std::endl;
	}

	int ID() const
	{
		return m_id;
	}

private:

	HPG LoadNext_main()
	{
		HPG pgNext = H_INVALID;
		if (m_pgSrcDirs.end() != m_itSrcDirs)
		{
			pgNext = posture_graph_load(m_itSrcDirs->c_str(), m_pgName.c_str());
			int n_theta = 0;
			if (VALID_HANDLE(pgNext))
			{
				n_theta = N_Theta(pgNext);
				m_nTheta0 += n_theta;
			}
			std::cout << m_id << ": Loading " << m_itSrcDirs->c_str() << " results " << n_theta << " postures." << std::endl;
			m_itSrcDirs = m_pgSrcDirs.erase(m_itSrcDirs);

		}
		return pgNext;
	}

	virtual void Run_worker()
	{
		if (VALID_HANDLE(m_hpgSrc0)
			&& VALID_HANDLE(m_hpgSrc1))
			m_hpgRes = posture_graph_merge(m_hpgSrc0, m_hpgSrc1, m_interest_conf.c_str(), m_epsErr);
	}

private:
	std::string m_interest_conf;
	std::string m_pgName;
	std::list<std::string> m_pgSrcDirs;
	std::list<std::string>::iterator m_itSrcDirs;
	std::string m_pgDstDir;
	HPG m_hpgSrc0;
	HPG m_hpgSrc1;
	HPG m_hpgRes;
	volatile Real m_epsErr;
	int m_nTheta0;
	int m_nPGs;
	int m_id;
};

void InitPGDirList(const std::string& dir_root, const std::string& pg_name, std::list<std::string>& dirs_pg)
{
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
		TraverseDirTree_filter(dir_root, OnDirPG, pg_name + ".pg" );
		TraverseDirTree_filter(dir_root, OnDirHTR, pg_name + ".htr" );
	}
	catch (std::string &info)
	{
		std::cout << "ERROR: " << info << std::endl;
	}

	class GreatorFileSize
	{
	public:
		GreatorFileSize(const std::string& pg_name)
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
	dirs_pg = std::move(dirs_src);
}

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (!(5 < argc))
	{
		std::cout << "Usage:\tposture_graph_merge_parallel <INTERESTS_XML> <PG_DIR_SRC> <PG_DIR_DST> <Epsilon> <PG_NAME>+ " << std::endl;
		return -1;
	}
	else
	{
		auto tick_start = ::GetTickCount64();

		std::string path_interests_conf = argv[1];
		std::string dir_src = argv[2];
		std::string dir_dst = argv[3];
		Real eps_err = (Real)atof(argv[4]);

		int i_pg_base = 5;
		int i_pg_end = argc;

		int n_pg = i_pg_end - i_pg_base;
		std::vector<std::string> pg_names(n_pg);
		std::vector<std::list<std::string>> pg_list_dirs(n_pg);
		for (int i_pg = 0; i_pg < n_pg; i_pg ++)
		{
			std::string pg_i(argv[i_pg + i_pg_base]);
			InitPGDirList(dir_src, pg_i, pg_list_dirs[i_pg]);
			pg_names[i_pg] = std::move(pg_i);
		}

		// for (int i_pg = 0; i_pg < n_pg; i_pg ++)
		// {
		// 	std::cout << pg_names[i_pg] << ":" << std::endl;
		// 	auto& dirs_pg_i = pg_list_dirs[i_pg];
		// 	for (auto dir_pg_i : dirs_pg_i)
		// 		std::cout << "\t\t" << dir_pg_i << std::endl;
		// }

		CThreadPool_W32<CMergeThread> pool;
		int id_pg = 0;
		pool.Initialize_main(n_pg,
							[&](CMergeThread* thread)
								{
									thread->Initialize(path_interests_conf,
														pg_names[id_pg],
														pg_list_dirs[id_pg],
														dir_dst,
														eps_err,
														id_pg);
									id_pg++;
								});

		bool* tasks_done = new bool[n_pg];
		memset(tasks_done, false, n_pg * sizeof(bool));

		auto AllDone = [](bool* tasks_done, int n_tasks)
			{
				bool all_done = true;
				for (int i_task = 0
					; i_task < n_tasks && all_done
					; i_task ++)
					all_done = tasks_done[i_task];
				return all_done;
			};

		while(!AllDone(tasks_done, n_pg))
		{
			CMergeThread* thread_i = pool.WaitForAReadyThread_main(INFINITE);
			bool merge_next = thread_i->MergeEnd_main();
			tasks_done[thread_i->ID()] = !merge_next;
			if (merge_next)
				thread_i->MergeStart_main();
			else
				thread_i->MergeComplete_main();
		}

		auto tick_cnt = ::GetTickCount64() - tick_start;
		printf("************TOTAL TIME: %.2f seconds*************\n"
			, (double)tick_cnt/(double)1000);
	}

	return 0;
}
