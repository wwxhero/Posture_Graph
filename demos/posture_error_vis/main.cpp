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

	if (3 != argc)
	{
		std::cout << "Usage:\tposture_error_vis <HTR> <PNG_grayscale>" << std::endl;
		return -1;
	}
	else
	{
		try
		{
			// std::cerr << "Usage:\tposture_error_vis <HTR> <PNG_grayscale>" << std::endl;
			const char* c_exts[] = { ".htr", ".png" };
			fs::path path_src(argv[1]);
			fs::path path_dst(argv[2]);
			std::string exts_input[] = { Norm(path_src.extension().u8string()), Norm(path_dst.extension().u8string()) };
			bool htr2png = (exts_input[0] == c_exts[0] && exts_input[1] == c_exts[1]);
			if (htr2png)
			{
				const char* path_htr = argv[1];
				const char* path_png = argv[2];
				auto tick_start = ::GetTickCount64();
				err_vis(path_htr, path_png);
				auto tick = ::GetTickCount64() - tick_start;
				float tick_sec = tick / 1000.0f;
				printf("Converting error-table from %s to %s takes %.2f seconds\n", argv[1], argv[2], tick_sec);
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
