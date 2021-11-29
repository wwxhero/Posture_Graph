#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include "posture_graph.h"
#include "error_table_helper.hpp"
#include "filesystem_helper.hpp"

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (4 != argc)
	{
		std::cout << "Usage:\tposture_error_vis <INTERESTS> <HTR> <PNG_grayscale>" << std::endl;
		return -1;
	}
	else
	{
		try
		{
			const char* c_exts[] = { ".xml", ".htr", ".png" };
			const char* path_interests_conf = argv[1];
			const char* path_src = argv[2];
			const char* path_dst = argv[3];
			std::string exts_input[] = {
				fs::path(path_interests_conf).extension().u8string(),
				fs::path(path_src).extension().u8string(),
				fs::path(path_dst).extension().u8string()
			};
			bool htr2png = (exts_input[0] == c_exts[0] 
							&& exts_input[1] == c_exts[1]
							&& exts_input[2] == c_exts[2]);
			if (htr2png)
			{
				const char* res[] = { "failed", "successful" };
				auto tick_start = ::GetTickCount64();
				_ERROR_TB etb;
				bool done = init_err_tb(path_interests_conf, path_src, &etb);
				if (done)
				{
					cv::Mat vis;
					ETB_Convert(&etb, vis);
					imwrite(path_dst, vis);
					uninit_err_tb(&etb);
				}
				int i_res = done ? 1 : 0;
				auto tick = ::GetTickCount64() - tick_start;
				float tick_sec = tick / 1000.0f;
				printf("Converting error-table from %s to %s takes %.2f seconds: %s\n", argv[1], argv[2], tick_sec, res[i_res]);
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
