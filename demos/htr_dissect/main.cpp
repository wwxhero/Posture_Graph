#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include <filesystem>
#include "posture_graph.h"
#include "filesystem_helper.hpp"

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (4 != argc)
	{
		std::cout << "Usage:\thtr_dissect <PG.XML> <BVH_DIR_SRC> <BVH_DIR_DST>" << std::endl;
		return -1;
	}
	else
	{
		typedef bool (*OnFile)(const char* path_src, const char* path_dst);
		const char* path_xml = argv[1];

		auto onhtr = [path_xml] (const char* path_src, const char* path_dst) -> bool
			{
				auto outDir = fs::path(path_dst).replace_extension();
				fs::create_directory(outDir);
				bool ok = dissect(path_xml, path_src, outDir.u8string().c_str());
				const char* res[] = { "failed", "successful" };
				int i_res = (ok ? 1 : 0);
				printf("dissect %s to %s: %s\n", path_src, path_dst, res[i_res]);
				return true;
			};

		try
		{
			const char* dir_src = argv[2];
			const char* dir_dst = argv[3];
			auto tick_start = ::GetTickCount64();
			CopyDirTree(dir_src, dir_dst, onhtr, ".htr");
			auto tick = ::GetTickCount64() - tick_start;
			float tick_sec = tick / 1000.0f;
			printf("Converting %s to %s takes %.2f seconds\n", dir_src, dir_dst, tick_sec);
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}
	}

	return 0;
}
