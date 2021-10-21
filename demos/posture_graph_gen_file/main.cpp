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

	if (3 != argc)
	{
		std::cout << "Usage:\tposture_graph_gen_file <HTR> <PG_DIR>" << std::endl;
		return -1;
	}
	else
	{
		try
		{
			const char* c_exts[] = {".htr"};
			const char* path_htr = argv[1];
			const char* pg_dir = argv[2];
			std::string exts_input[] = {
				Norm(fs::path(path_htr).extension().u8string()),
			};
			bool htr2pg = (exts_input[0] == c_exts[0]);
			if (htr2pg)
			{
				const char* res[] = { "failed", "successful" };
				auto tick_start = ::GetTickCount64();
				bool done = posture_graph_gen(path_htr, pg_dir);
				int i_res = done ? 1 : 0;
				auto tick = ::GetTickCount64() - tick_start;
				float tick_sec = tick / 1000.0f;
				printf("Converting error-table from %s to %s takes %.2f seconds: %s\n", path_htr, pg_dir, tick_sec, res[i_res]);
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
