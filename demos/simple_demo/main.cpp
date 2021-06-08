#include <iostream>
#include <bvh11.hpp>
#include "articulated_body.h"
bool ResetRestPose(bvh11::BvhObject& bvh, int i_frame)
{
	float theta = 0;
	float theta_prime = ik_test(theta);
	std::cout << "theta = " << theta << "; theta_prime = " << theta_prime << std::endl;
	return false;
}

int main(int argc, char* argv[])
{
	bool for_show_file_info = (2 == argc);
	bool for_reset_restpose = (4 == argc);
	if (!for_show_file_info
	 && !for_reset_restpose)
	{
		std::cerr << "Usage:\tsimpe_demo [BVH_PATH]\t\t//to show the file information" << std::endl;
		std::cerr <<       "\tsimpe_demo [BVH_PATH_SRC] [FRAME_NO] [BVH_PATH_DEST]\t//to reset rest posture with the given frame posture" << std::endl;

	}
	else
	{
		const std::string bvh_file_path = argv[1];

		bvh11::BvhObject bvh(bvh_file_path);

		if (for_show_file_info)
		{
			std::cout << "#Channels       : " << bvh.channels().size() << std::endl;
			std::cout << "#Frames         : " << bvh.frames()          << std::endl;
			std::cout << "Frame time      : " << bvh.frame_time()      << std::endl;
			std::cout << "Joint hierarchy : " << std::endl;
			bvh.PrintJointHierarchy();
		}
		else
		{
			const std::string bvh_file_path_dst = argv[3];
			int n_frame = atoi(argv[2]);
			if (ResetRestPose(bvh, n_frame - 1))
				bvh.WriteBvhFile(bvh_file_path_dst);
			else
			{
				std::cout << "Please input a right frame number between 1 and " << bvh.frames() << " !!!" << std::endl;
			}
		}
	}



	return 0;
}
