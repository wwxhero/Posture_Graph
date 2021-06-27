#include <iostream>
#include <bvh11.hpp>
#include "bvh.h"



int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	bool for_show_file_info = (2 == argc);
	bool for_reset_restpose = (4 == argc);
	if (!for_show_file_info
	 && !for_reset_restpose)
	{
		std::cerr << "Usage:\tsimpe_demo <BVH_PATH>\t\t//to show the file information" << std::endl;
		std::cerr <<       "\tsimpe_demo <BVH_PATH_SRC> <FRAME_NO> <BVH_PATH_DEST>\t//to reset rest posture with the given frame posture" << std::endl;

	}
	else
	{
		const std::string bvh_file_path = argv[1];
		if (for_show_file_info)
		{
			auto tick_start = ::GetTickCount64();
			bvh11::BvhObject bvh(bvh_file_path);
			auto tick = ::GetTickCount64() - tick_start;
			float tick_sec = tick / 1000.0f;
			printf("Parsing %s takes %.2f seconds\n", bvh_file_path.c_str(), tick_sec);

			std::cout << "#Channels       : " << bvh.channels().size() << std::endl;
			std::cout << "#Frames         : " << bvh.frames()          << std::endl;
			std::cout << "Frame time      : " << bvh.frame_time()      << std::endl;
			std::cout << "Joint hierarchy : " << std::endl;
			bvh.PrintJointHierarchy();
#ifdef _DEBUG
			std::string bvh_file_path_dup(bvh_file_path);
			bvh_file_path_dup += "_dup";
			bvh.WriteBvhFile(bvh_file_path_dup);
#endif
		}
		else
		{
			const std::string bvh_file_path_dst = argv[3];
			int n_frame = atoi(argv[2]);
			auto tick_start = ::GetTickCount64();
			bool resetted = ResetRestPose(bvh_file_path.c_str()
										, n_frame - 1
										, bvh_file_path_dst.c_str());
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
