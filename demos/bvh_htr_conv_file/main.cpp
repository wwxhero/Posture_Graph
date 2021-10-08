#include <windows.h>
#include <string>
#include <iostream>
#include "bvh.h"
#include "filesystem_helper.hpp"


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (3 != argc)
	{
		std::cerr << "Usage:\tbvh_htr_conv_file <PATH_SRC> <PATH_DST>\t\t//to convert file <PATH_SRC> to <PATH_DST>" << std::endl;
	}
	else
	{
		auto convert = [](const char* src, const char* dst, bool htr2bvh)
		{
			std::cout << src << "\t" << dst << "\t" << "htr2bvh=" << htr2bvh << std::endl;
		};
		const char* c_exts[] = { ".htr", ".bvh" };
		fs::path path_src(argv[1]);
		fs::path path_dst(argv[2]);
		std::string exts_input[] = { Norm(path_src.extension().u8string()), Norm(path_dst.extension().u8string()) };
		bool htr2bvh = (exts_input[0] == c_exts[0] && exts_input[1] == c_exts[1]);
		bool bvh2htr = (exts_input[0] == c_exts[1] && exts_input[1] == c_exts[0]);
		if (htr2bvh || bvh2htr)
			convert(argv[1], argv[2], htr2bvh);
		else
		{
			std::cout << "Not the right file extensions!!!" << std::endl;
		}
	}
	return 0;
}
