#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include "filesystem_helper.hpp"
#include "posture_graph.h"
#include "parallel_thread_helper.hpp"


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

		for (auto path: dirs_src)
			std::cout << path << std::endl;


		auto tick_cnt = ::GetTickCount64() - tick_start;
		printf("************TOTAL TIME: %.2f seconds*************\n", (double)tick_cnt/(double)1000);

	}



	return 0;
}
