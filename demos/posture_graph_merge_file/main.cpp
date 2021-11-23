#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include "filesystem_helper.hpp"
#include "posture_graph.h"


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (7 != argc)
	{
		std::cout << "Usage:\tposture_graph_merge_file <INTEREST_XML> <PG_DIR_0> <PG_DIR_1> <PG_NAME> <Epsilon> <PG_DIR>" << std::endl;
		return -1;
	}
	else
	{
		try
		{
			const char* path_interests_conf = argv[1];
			const char* pg_dir_0 = argv[2];
			const char* pg_dir_1 = argv[3];
			const char* pg_name = argv[4];
			Real eps_err = (Real)atof(argv[5]);
			const char* pg_dir_dst = argv[6];

			printf("Merging %s and %s for files %s.htr and %s.pg ", pg_dir_0, pg_dir_1, pg_name, pg_name);
			const char* res[] = { "failed", "successful" };
			auto tick_start = ::GetTickCount64();

			bool done = false;
			HPG hpg_0 = posture_graph_load(pg_dir_0, pg_name);
			HPG hpg_1 = posture_graph_load(pg_dir_1, pg_name);
			if (VALID_HANDLE(hpg_0)
				&& VALID_HANDLE(hpg_1))
			{
				HPG hpg = posture_graph_merge(hpg_0, hpg_1, path_interests_conf, eps_err);
				if (VALID_HANDLE(hpg))
				{
					done = save_pg(hpg, pg_dir_dst);
					posture_graph_release(hpg);
				}
				posture_graph_release(hpg_0);
				posture_graph_release(hpg_1);
			}

			int i_res = done ? 1 : 0;
			auto tick = ::GetTickCount64() - tick_start;
			float tick_sec = tick / 1000.0f;
			printf("takes %.2f seconds: %s\n", tick_sec, res[i_res]);
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}
	}



	return 0;
}
