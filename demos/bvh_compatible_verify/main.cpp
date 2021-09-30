#include <iostream>
#include <vector>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include "articulated_body.h"
#include "bvh.h"

typedef bool (*CB_OnBVHFile)(const char* path);


void TraverseDirTree(const char* dirPath, CB_OnBVHFile onbvh) //  throw (std::string)
{
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	TCHAR szDir[MAX_PATH];
	size_t length_of_arg;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError=0;

	// If the directory is not specified as a command-line argument,
	// print usage.

	// Check that the input path plus 3 is not longer than MAX_PATH.
	// Three characters are for the "\*" plus NULL appended below.

	StringCchLength(dirPath, MAX_PATH, &length_of_arg);

	if (length_of_arg > (MAX_PATH - 3))
		throw std::string("\nDirectory path is too long.\n");

	// Prepare string for use with FindFile functions.  First, copy the
	// string to a buffer, then append '\*' to the directory name.

	StringCchCopy(szDir, MAX_PATH, dirPath);
	StringCchCat(szDir, MAX_PATH, TEXT("\\*"));

	// Find the first file in the directory.

	hFind = FindFirstFile(szDir, &ffd);

	if (INVALID_HANDLE_VALUE == hFind)
	{
		throw std::string("FindFirstFile: ") + std::string(dirPath);
	}

	// List all the files in the directory with some info about them.
	std::vector<std::string> trivial_dir = {".", ".."};
	bool traversing = true;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;
		else if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			bool trivial = false;
			for (auto dir_t : trivial_dir)
			{
				trivial = (dir_t == std::string(ffd.cFileName));
				if (trivial)
					break;
			}
			if (!trivial)
			{// _tprintf(TEXT("  %s   <DIR>\n"), ffd.cFileName);
				try
				{
					PathCombine(szDir, dirPath, ffd.cFileName);
					TraverseDirTree(szDir, onbvh);
				}
				catch (std::string& info)
				{
					FindClose(hFind);
					traversing = false;
					throw info;
				}
			}
		}
		else
		{
			filesize.LowPart = ffd.nFileSizeLow;
			filesize.HighPart = ffd.nFileSizeHigh;
			const char c_append[] = "hvb.";
			const int len_append = sizeof(c_append) / sizeof(const char) - 1;
			StringCchLength(ffd.cFileName, MAX_PATH, &length_of_arg);
			if (!(length_of_arg < len_append))
			{
				const char* s_append = ffd.cFileName + length_of_arg;
				bool matched = true;
				int i = 0; s_append--;
				for (
					; i < len_append && (matched = (tolower(*s_append) == c_append[i]))
					; i ++ , s_append --);
				if (matched)
					//_tprintf(TEXT("  %s   %ld bytes\n"), ffd.cFileName, filesize.QuadPart);
					traversing = onbvh(ffd.cFileName);
			}
		}
	}
	while (traversing
		&& FindNextFile(hFind, &ffd) != 0);

	FindClose(hFind);
	dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES)
	{
		throw std::string("FindFirstFile");
	}


}


int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	bool for_testing_compatibility = (3 == argc);
	bool for_reset_restpose = (4 == argc || 5 == argc);
	if (!for_testing_compatibility
	 && !for_reset_restpose)
	{
		std::cerr << "Usage:\tbvh_compatible_verify <STANDARD_BVH_PATH> <BVH_DIR>\t\t//to show the file information" << std::endl;
		// std::cerr <<       "\tsimpe_demo <BVH_DIR_SRC> <BVH_PATH_DEST> <FRAME_NO> [<SCALE>]\t//to reset rest posture with the given frame posture" << std::endl;
	}
	else
	{
		const std::string bvh_file_path = argv[1];
		if (for_testing_compatibility)
		{
			auto tick_start = ::GetTickCount64();
			HBVH hBVH_s = load_bvh_c(bvh_file_path.c_str());
			auto tick = ::GetTickCount64() - tick_start;
			float tick_sec = tick / 1000.0f;
			printf("Parsing %s takes %.2f seconds\n", bvh_file_path.c_str(), tick_sec);

			std::cout << "#Channels       : " << channels(hBVH_s)		<< std::endl;
			std::cout << "#Frames         : " << get_n_frames(hBVH_s)		<< std::endl;
			std::cout << "Frame time      : " << frame_time(hBVH_s) 	<< std::endl;
			std::cout << "Joint hierarchy : " << std::endl;
			PrintJointHierarchy(hBVH_s);
#ifdef _DEBUG
			std::string bvh_file_path_dup(bvh_file_path);
			bvh_file_path_dup += "_dup";
			WriteBvhFile(hBVH_s, bvh_file_path_dup.c_str());
#endif
			HBODY body_s = create_tree_body_bvh(hBVH_s);

			std::cout << "BVH files:" << std::endl;
			auto onbvh = [] (const char* path) -> bool
				{
					std::cout << path << std::endl;
					return true;
				};
			try
			{
				TraverseDirTree(argv[2], onbvh);
			}
			catch (std::string &info)
			{
				std::cout << "ERROR: " << info << std::endl;
			}

			destroy_tree_body(body_s);
			unload_bvh(hBVH_s);
		}
		else
		{
			const std::string bvh_file_path_dst = argv[2];
			int i_frame = atoi(argv[3]);
			double scale = ((5 == argc)
							? atof(argv[4])
							: 1);
			auto tick_start = ::GetTickCount64();
			bool resetted = ResetRestPose(bvh_file_path.c_str()
										, i_frame
										, bvh_file_path_dst.c_str()
										, scale);
			auto tick = ::GetTickCount64() - tick_start;
			auto tick_sec = tick / 1000.0f;
			const char* results[] = {"failed" , "successful"};
			int i_result = (resetted ? 1 : 0);
			printf("Reset posture takes %.2f seconds %s\n",
					tick_sec,
					results[i_result]);
		}
	}



	return 0;
}
