#include <iostream>
#include "bvh.h"



int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	bool for_show_file_info = (2 == argc);
	bool for_reset_restpose = (4 == argc || 5 == argc);
	if (!for_show_file_info
	 && !for_reset_restpose)
	{
		std::cerr << "Usage:\tsimpe_demo <BVH_PATH>\t\t//to show the file information" << std::endl;
		std::cerr <<       "\tsimpe_demo <BVH_PATH_SRC> <BVH_PATH_DEST> <FRAME_NO> [<SCALE>]\t//to reset rest posture with the given frame posture" << std::endl;
	}
	else
	{
		const std::string bvh_file_path = argv[1];
		if (for_show_file_info)
		{
			auto tick_start = ::GetTickCount64();
			HBVH hBVH = load_bvh_c(bvh_file_path.c_str());
			auto tick = ::GetTickCount64() - tick_start;
			float tick_sec = tick / 1000.0f;
			printf("Parsing %s takes %.2f seconds\n", bvh_file_path.c_str(), tick_sec);

			std::cout << "#Channels       : " << channels(hBVH)		<< std::endl;
			std::cout << "#Frames         : " << get_n_frames(hBVH)		<< std::endl;
			std::cout << "Frame time      : " << frame_time(hBVH) 	<< std::endl;
			std::cout << "Joint hierarchy : " << std::endl;
			PrintJointHierarchy(hBVH);
#ifdef _DEBUG
			std::string bvh_file_path_dup(bvh_file_path);
			bvh_file_path_dup += "_dup";
			WriteBvhFile(hBVH, bvh_file_path_dup.c_str());
#endif
			unload_bvh(hBVH);
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
