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
		std::cout << "Usage:\tposture_graph_gen <INTERESTS_XML> <BVH_DIR_SRC> <BVH_DIR_DST> <Epsilon>" << std::endl;
		return -1;
	}
	else
	{
		const char* path_interests_conf = argv[1];
		const char* dir_src = argv[2];
		const char* dir_dst = argv[3];
		Real eps_err = (Real)atof(argv[4]);


		auto onhtr = [path_interests_conf, eps_err] (const char* path_src, const char* path_dst) -> bool
			{
				std::string dir_dst = fs::path(path_dst).parent_path().u8string();
				printf("Building Posture-Graph from %s to %s ", path_src, dir_dst.c_str());
				auto tick_start = ::GetTickCount64();
				bool built = posture_graph_gen(path_interests_conf, path_src, dir_dst.c_str(), eps_err, NULL);
				const char* res[] = { "failed", "successful" };
				int i_res = (built ? 1 : 0);
				auto tick = ::GetTickCount64() - tick_start;
				float tick_sec = tick / 1000.0f;
				printf("takes %.2f seconds: %s\n", tick_sec, res[i_res]);
				return true;
			};
		try
		{
			CopyDirTree(dir_src, dir_dst, onhtr, ".htr");
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}
	}



	return 0;
}
