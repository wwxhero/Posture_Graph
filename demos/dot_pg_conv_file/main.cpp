#include <windows.h>
#include <string>
#include <iostream>
#include "posture_graph.h"
#include "filesystem_helper.hpp"


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (3 != argc)
	{
		std::cerr << "Usage:\tdot_pg_conv_file <PATH_SRC> <PATH_DST>\t\t//to convert file <PATH_SRC> to <PATH_DST>" << std::endl;
	}
	else
	{
		const char* c_exts[] = { ".pg", ".dot" };
		fs::path path_src(argv[1]);
		fs::path path_dst(argv[2]);
		std::string exts_input[] = { Norm(path_src.extension().u8string()), Norm(path_dst.extension().u8string()) };
		bool pg2dot = (exts_input[0] == c_exts[0] && exts_input[1] == c_exts[1]);
		if (pg2dot)
		{
			auto tick_start = ::GetTickCount64();
			bool converted = convert_pg2dot(argv[1], argv[2]);
			auto tick = ::GetTickCount64() - tick_start;
			float tick_sec = tick / 1000.0f;
			const char* res[] = { "failed", "successful" };
			int i_res = (converted ? 1 : 0);
			printf("Converting %s to %s takes %.2f seconds: %s\n", argv[1], argv[2], tick_sec, res[i_res]);
		}
		else
		{
			std::cout << "Not the right file extensions!!!" << std::endl;
		}
	}
	return 0;
}
