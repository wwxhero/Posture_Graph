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

void conv(const char* path_src, const char* path_dst, bool htr2bvh)
{
	bool converted = convert(path_src, path_dst, htr2bvh);
	const char* res[] = { "failed", "successful" };
	int i_res = (converted ? 1 : 0);
	printf("Converting %s to %s: %s\n", path_src, path_dst, res[i_res]);
};

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (3 != argc)
	{
		std::cout << "Usage:\tbvh_posture_rest <BVH_DIR_SRC> <BVH_DIR_DST>" << std::endl;
		return -1;
	}
	else
	{
		const char* dir_src = argv[1];
		const char* dir_dst = argv[2];

		typedef bool (*OnFile)(const char* path_src, const char* path_dst);

		OnFile onbvh = [] (const char* path_src, const char* path_dst) -> bool
			{
				auto path_dst_htr = fs::path(path_dst).replace_extension(fs::path("htr"));
				// std::cout << "onbvh: " <<  path_src << "\t" << path_dst_htr.generic_u8string() << std::endl;
				conv(path_src, path_dst_htr.generic_u8string().c_str(), false);
				return true;
			};

		OnFile onhtr = [] (const char* path_src, const char* path_dst) -> bool
			{
				auto path_dst_bvh = fs::path(path_dst).replace_extension(fs::path("bvh"));
				// std::cout << "onhtr: " << path_src << "\t" << path_dst_htr.generic_u8string() << std::endl;
				conv(path_src, path_dst_bvh.generic_u8string().c_str(), true);
				return true;
			};
		try
		{
			std::vector<OnFile> onFile = {onbvh, onhtr};
			std::vector<std::string> exts = {".bvh", ".htr"};
			CopyDirTree(dir_src, dir_dst, onFile, exts);
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}
	}



	return 0;
}
