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

	if (5 != argc)
	{
		std::cout << "Usage:\tposture_graph_gen_file <INTEREST_XML> <HTR> <PG_DIR> <Epsilon>" << std::endl;
		return -1;
	}
	else
	{
		try
		{
			const char* c_exts[] = {".htr"};
			const char* path_interests_conf = argv[1];
			const char* path_htr = argv[2];
			const char* pg_dir = argv[3];
			Real eps_err = (Real)atof(argv[4]);
			std::string exts_input[] = {
				Norm(fs::path(path_htr).extension().u8string()),
			};
			bool htr2pg = (exts_input[0] == c_exts[0]);
			if (htr2pg)
			{
				printf("Converting error-table from %s to %s ", path_htr, pg_dir);
				const char* res[] = { "failed", "successful" };
				auto tick_start = ::GetTickCount64();
				bool done = posture_graph_gen(path_interests_conf, path_htr, pg_dir, eps_err, NULL);
				int i_res = done ? 1 : 0;
				auto tick = ::GetTickCount64() - tick_start;
				float tick_sec = tick / 1000.0f;
				printf("takes %.2f seconds: %s\n", tick_sec, res[i_res]);
			}
			else
			{
				std::cout << "Not the right file extensions!!!" << std::endl;
			}
			
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}
	}



	return 0;
}
