#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include "articulated_body.h"
#include "bvh.h"
#include "filesystem_helper.hpp"
#include "posture_graph.h"

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (argc < 4)
	{
		std::cout << "Usage:\thtr_bvh_trim <FILE_SRC> <FILE_DST> <SubTreeRM>+ \t\t\t\t\t// to trim the articulated body tree file <FILE_SRC>, the trimming result is stored in <FILE_DST>" << std::endl;
	}
	else
	{
		const int i_base_subtree = 3;
		int n_subtrees_rm = argc-i_base_subtree;
		std::vector<const char*> name_subtrees_rm(n_subtrees_rm);
		for (int i_subtree = 0; i_subtree < n_subtrees_rm; i_subtree ++)
			name_subtrees_rm[i_subtree] = argv[i_subtree + i_base_subtree];

		const char* path_src = argv[1];
		const char* path_dst = argv[2];

		try
		{
			std::cout << "Trimming " << path_src << " to " << path_dst << ":";
			bool trimmed = trim(path_src, path_dst, name_subtrees_rm.data(), (int)name_subtrees_rm.size());
			if (trimmed)
				std::cout << "successful!" << std::endl;
			else
				std::cout << "failed!!!" << std::endl;
		}
		catch(const std::string& exp)
		{
			std::cout << exp;
		}

	}



	return 0;
}
