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

	bool for_testing_compatibility = (3 == argc);
	bool for_testing_restpose_T = (2 == argc);
	if (!for_testing_compatibility
	 && !for_testing_restpose_T)
	{
		std::cout << "Usage:\tbvh_compatible_verify <STANDARD_BVH> <BVH_DIR>\t\t\t\t\t//to compare the files in <BVH_DIR> against <STANDARD_BVH>" << std::endl;
		std::cout << 	   "\tbvh_compatible_verify <BVH_DIR>\t\t\t\t\t\t\t//to verify for the files in <BVH_DIR> in 'T' posture" << std::endl;
	}
	else
	{

		std::vector<const char*> right_arm_pts = {
			"RightForeArm", "RightHand"						//right arm points
		};
		std::vector<const char*> left_arm_pts = {
			"LeftForeArm", "LeftHand"						//left arm points
		};
		std::vector<const char*> spine_pts = {
			"Spine", "Spine1", "Neck", "Neck1", "Head",		//spine points
		};
		std::vector<const char*> right_leg_pts = {
			"RightFoot", "RightLeg",						//right leg points
		};
		std::vector<const char*> left_leg_pts = {
			"LeftFoot", "LeftLeg", 							//left leg points
		};

		std::vector<const char*> pts_interest;
		std::vector<const char*>* parts[] = {&spine_pts, &right_leg_pts, &left_leg_pts, &right_arm_pts, &left_arm_pts };
		for (auto part : parts)
		{
			pts_interest.insert(pts_interest.end(), part->begin(), part->end());
		}
		int n_interests = (int)pts_interest.size();
		Real* err = new Real[n_interests];
		const Real c_errMax = (Real)180;

		if (for_testing_compatibility)
		{
			auto tick_start = ::GetTickCount64();
			const std::string bvh_file_path = argv[1];
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

			std::cout << "standard BVH file:" << argv[1] << std::endl;
			HBODY body_s = create_tree_body_bvh(hBVH_s);
			destroy_tree_body(body_s);
			unload_bvh(hBVH_s);
		}
		else // for_testing_restpose_T
		{
			auto onbvh = [&] (const char* path) -> bool
			{
				HBVH hBVH_d = load_bvh_c(path);
				HBODY body_d = H_INVALID;
				int n_nT_nodes = 0;
				const Real up[] = {(Real)0, (Real)1, (Real)0}; //+Y
				// memset(err, c_errMax, n_interests * sizeof(Real));
				for (int i_interest = 0; i_interest < n_interests; i_interest++)
					err[i_interest] = c_errMax;
				if (VALID_HANDLE(hBVH_d)
					&& VALID_HANDLE(body_d = create_tree_body_bvh(hBVH_d)))
				{
					int i_sub_parts[5][2] = {0};
					const int n_parts = 5;
					i_sub_parts[0][0] = 0;
					i_sub_parts[0][1] = (int)parts[0]->size();
					for (int i_part = 1; i_part < n_parts; i_part ++)
					{
						int& i_sub_part_start = i_sub_parts[i_part][0];
						int& i_sub_part_end = i_sub_parts[i_part][0];
						const int i_sub_part_end_p = i_sub_parts[i_part-1][1];
						i_sub_part_start = i_sub_part_end_p;
						i_sub_part_end = i_sub_part_start + (int)parts[i_part]->size();
					}

					body_T_test(body_d, up
						, pts_interest.data(), (int)pts_interest.size()
						, i_sub_parts
						, err);
				}

				std::cout << path;
				for (int i_err = 0; i_err < n_interests; i_err ++)
					std::cout << "\t" << err[i_err];
				std::cout << std::endl;

				if (VALID_HANDLE(body_d))
					destroy_tree_body(body_d);
				if (VALID_HANDLE(hBVH_d))
					unload_bvh(hBVH_d);
				return true;
			};
			try
			{
				const std::string bvh_file_dir = argv[1];
				std::cout << "File";
				for (int i_interest = 0; i_interest < n_interests; i_interest ++)
					std::cout << "\t" << pts_interest[i_interest];
				std::cout << std::endl;
				TraverseDirTree(bvh_file_dir, onbvh, ".bvh");
			}
			catch (std::string &info)
			{
				std::cout << "ERROR: " << info << std::endl;
			}
		}
		delete [] err;





	}



	return 0;
}
