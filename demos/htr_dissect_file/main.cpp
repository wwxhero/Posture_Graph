#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include "posture_graph.h"
#include "filesystem_helper.hpp"

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (4 != argc)
	{
		std::cout << "Usage:\thtr_dissect_file <PG.XML> <HTR> <OutDir>" << std::endl;
		return -1;
	}
	else
	{
		try
		{
			const char* c_exts[] = { ".xml", ".htr" };
			fs::path path_src(argv[1]);
			fs::path path_dst(argv[2]);
			std::string exts_input[] = { Norm(path_src.extension().u8string()), Norm(path_dst.extension().u8string()) };
			bool correct_exts = (exts_input[0] == c_exts[0] && exts_input[1] == c_exts[1]);
			if (correct_exts)
			{
				const char* path_xml = argv[1];
				const char* path_htr = argv[2];
				const char* dir_out = argv[3];
				auto tick_start = ::GetTickCount64();
				dissect(path_xml, path_htr, dir_out);
				auto tick = ::GetTickCount64() - tick_start;
				float tick_sec = tick / 1000.0f;
				printf("dissecting %s takes %.2f seconds\n", path_htr, tick_sec);
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
