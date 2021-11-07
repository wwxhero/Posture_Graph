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
		// std::cout << "Usage:\tbvh_compatible_verify <STANDARD_BVH> <BVH_DIR>\t\t\t\t\t//to compare the files in <BVH_DIR> against <STANDARD_BVH>" << std::endl;
		std::cout << 	   "\tbvh_compatible_verify <BVH_DIR>\t\t\t\t\t\t\t//to verify for the files in <BVH_DIR> in 'T' posture" << std::endl;
	}
	else
	{

		const Real c_errMax = (Real)180;

		if (for_testing_compatibility)
		{
			auto tick_start = ::GetTickCount64();
			const std::string bvh_file_path = argv[1];
			HBVH hBVH_s = load_bvh_c(bvh_file_path.c_str());
			if (!VALID_HANDLE(hBVH_s))
			{
				return -1;
			}
			HBODY body_s = create_tree_body_bvh(hBVH_s);

			int n_interests = 0;
			HBODY* interests = alloc_bodies(body_s, &n_interests);
			Real* errs = new Real[n_interests];
			auto onbvh = [&] (const char* path) -> bool
			{
				HBVH hBVH_d = load_bvh_c(path);
				HBODY body_d = H_INVALID;
				for (int i_interest = 0; i_interest < n_interests; i_interest++)
				 	errs[i_interest] = c_errMax;
				int n_errs = 0;
				if (VALID_HANDLE(hBVH_d)
				 	&& VALID_HANDLE(body_d = create_tree_body_bvh(hBVH_d)))
				{
				 	// body_EQ_test(interests, body_d, errs);
				}

				std::cout << path;
				for (int i_err = 0; i_err < n_interests; i_err ++)
				 	std::cout << "\t" << errs[i_err];
				std::cout << std::endl;

				if (VALID_HANDLE(body_d))
				 	destroy_tree_body(body_d);
				if (VALID_HANDLE(hBVH_d))
				 	unload_bvh(hBVH_d);
				return true;
			};
			try
			{
				const std::string bvh_file_dir = argv[2];
				std::cout << "File";
				for (int i_interest = 0; i_interest < n_interests; i_interest ++)
					std::cout << "\t" << body_name_c(interests[i_interest]);
				std::cout << std::endl;

				std::cout << bvh_file_path;
				for (int i_interest = 0; i_interest < n_interests; i_interest ++)
					std::cout << "\t0";
				std::cout << std::endl;
				TraverseDirTree(bvh_file_dir, onbvh, ".bvh");
			}
			catch (std::string &info)
			{
				std::cout << "ERROR: " << info << std::endl;
			}

			free_bodies(interests);
			delete [] errs;

			destroy_tree_body(body_s);
			unload_bvh(hBVH_s);
		}
		else // for_testing_restpose_T
		{
			std::vector<const char*> right_arm_pts = {
				"RightForeArm", "RightHand"						//right arm points
			};
			std::vector<const char*> left_arm_pts = {
				"LeftForeArm", "LeftHand"						//left arm points
			};
			std::vector<const char*> spine_pts = {
				"Hips", "LowerBack", "Spine", "Spine1", "Neck", "Neck1", "Head",		//spine points
			};
			std::vector<const char*> right_leg_pts = {
				"RightFoot", "RightLeg",						//right leg points
			};
			std::vector<const char*> left_leg_pts = {
				"LeftFoot", "LeftLeg", 							//left leg points
			};

			std::vector<const char*> pts_interest;
			std::vector<const char*>* parts[] = {&spine_pts, &right_leg_pts, &left_leg_pts, &right_arm_pts, &left_arm_pts };
			const int n_parts = sizeof(parts)/sizeof(std::vector<const char*>*);
			for (auto part : parts)
			{
				pts_interest.insert(pts_interest.end(), part->begin(), part->end());
			}
			int n_interests = (int)pts_interest.size();
			Real* err = new Real[n_interests];
			auto onbvh = [&] (const char* path) -> bool
			{
				HBVH hBVH_d = load_bvh_c(path);
				HBODY body_d = H_INVALID;
				const Real up[] = {(Real)0, (Real)1, (Real)0}; //+Y
				const Real forward[] = {(Real)0, (Real)0, (Real)1};
				// memset(err, c_errMax, n_interests * sizeof(Real));
				for (int i_interest = 0; i_interest < n_interests; i_interest++)
					err[i_interest] = c_errMax;
				if (VALID_HANDLE(hBVH_d)
					&& VALID_HANDLE(body_d = create_tree_body_bvh(hBVH_d)))
				{
					int (*parts_body_idx_range)[2] = (int (*)[2])malloc(2*n_parts*sizeof(int));
					parts_body_idx_range[0][0] = 0;
					parts_body_idx_range[0][1] = (int)parts[0]->size();
					for (int i_part = 1; i_part < n_parts; i_part ++)
					{
						int& i_part_start = parts_body_idx_range[i_part][0];
						int& i_part_end = parts_body_idx_range[i_part][1];
						const int i_part_end_p = parts_body_idx_range[i_part-1][1];
						i_part_start = i_part_end_p;
						i_part_end = i_part_start + (int)parts[i_part]->size();
					}

					body_T_test(body_d, up, forward
						, pts_interest.data(), (int)pts_interest.size()
						, parts_body_idx_range
						, err);
					free(parts_body_idx_range);
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
			delete[] err;
		}

	}



	return 0;
}
