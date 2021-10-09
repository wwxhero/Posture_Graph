#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include <filesystem>
#include "articulated_body.h"
#include "bvh.h"
#include "filesystem_helper.hpp"

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (4 != argc)
	{
		std::cout << "Usage:\tbvh_posture_rest <BVH_DIR_SRC> <BVH_DIR_DST> <POSE_ID>" << std::endl;
		return -1;
	}
	else
	{
		const char* dir_src = argv[1];
		const char* dir_dst = argv[2];
		int pose_id = atoi(argv[3]);

		auto onbvh = [pose_id] (const char* path_src, const char* path_dst) -> bool
			{
				bool resetted = ResetRestPose(path_src
											, pose_id
											, path_dst
											, 1.0);
				if (!resetted)
					std::cout << "reset pose from " << path_src << " to " << path_dst << " failed!!!" << std::endl;
				return true;
			};
		try
		{
			// std::cerr << "Usage:\tbvh_posture_rest <BVH_DIR_SRC> <BVH_DIR_DST> <POSE_ID>" << std::endl;
			CopyDirTree(dir_src, dir_dst, onbvh, ".bvh");
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}
	}



	return 0;
}
