#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include <filesystem>
#include "articulated_body.h"
#include "bvh.h"

template<typename LAMBDA_onext>
void TraverseDirTree(const std::string& dirPath, LAMBDA_onext onbvh, const std::string& ext) //  throw (std::string)
{
	namespace fs = std::experimental::filesystem;
	WIN32_FIND_DATA ffd;
	LARGE_INTEGER filesize;
	std::string filter = dirPath + "\\*";
	HANDLE hFind = INVALID_HANDLE_VALUE;
	DWORD dwError=0;

	// Find the first file in the directory.
	hFind = FindFirstFile(filter.c_str(), &ffd);
	if (INVALID_HANDLE_VALUE == hFind)
	{
		throw std::string("FindFirstFile: ") + dirPath;
	}
	// List all the files in the directory with some info about them.
	std::set<std::string> trivial_dir = {".", ".."};
	bool traversing = true;
	do
	{
		if (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN)
			continue;
		else if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			bool trivial = (trivial_dir.end() != trivial_dir.find(ffd.cFileName));

			if (!trivial)
			{
				try
				{
					fs::path dirPath_prime = fs::path(dirPath)/ffd.cFileName;
					TraverseDirTree(dirPath_prime.u8string(), onbvh, ext);
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

			if (fs::path(ffd.cFileName).extension().u8string() == ext)
			{
				fs::path filepath = fs::path(dirPath)/ffd.cFileName;
				traversing = onbvh(filepath.u8string().c_str());
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
			if (!VALID_HANDLE(hBVH_s))
			{
				std::cout << "file: " << bvh_file_path << " can not be opened!!!" << std::endl;
				return -1;
			}
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
			int n_err_nodes_cap = channels(hBVH_s);
			HBODY* err_nodes = (HBODY*)malloc(sizeof(HBODY) * n_err_nodes_cap);
			std::cout << "standard BVH file:" << argv[1] << std::endl;
			auto onbvh = [body_s, err_nodes, n_err_nodes_cap] (const char* path) -> bool
				{
					HBVH hBVH_d = load_bvh_c(path);
					HBODY body_d = H_INVALID;
					int n_err_nodes = 0;
					bool eq = VALID_HANDLE(hBVH_d)
								&& VALID_HANDLE(body_d = create_tree_body_bvh(hBVH_d))
								&& (0 == (n_err_nodes = body_cmp(body_s, body_d, err_nodes, n_err_nodes_cap))); //verify body_s == body_d
					const char * res[] = { "false", "true" };
					int i_res = (eq ? 1 : 0);
					std::cout << path << ": " << "Equal = " << res[i_res];
					for (int i_err_node = 0; i_err_node < n_err_nodes; i_err_node ++)
					{
						std::cout << "\t" << body_name_c(err_nodes[i_err_node]);
					}
					std:: cout << std::endl;
					
					if (VALID_HANDLE(body_d))
						destroy_tree_body(body_d);
					if (VALID_HANDLE(hBVH_d))
						unload_bvh(hBVH_d);
					return true;
				};
			try
			{
				TraverseDirTree(argv[2], onbvh, ".bvh");
			}
			catch (std::string &info)
			{
				std::cout << "ERROR: " << info << std::endl;
			}
			free(err_nodes);
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
