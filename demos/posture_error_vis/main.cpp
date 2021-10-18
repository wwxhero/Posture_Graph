#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <shlwapi.h>
#include <strsafe.h>
#include <opencv2/opencv.hpp>
#include "articulated_body.h"
#include "bvh.h"
#include "posture_graph.h"
#include "filesystem_helper.hpp"

int main(int argc, char* argv[])
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);

	if (3 != argc)
	{
		std::cout << "Usage:\tposture_error_vis <HTR> <PNG_grayscale>" << std::endl;
		return -1;
	}
	else
	{
		try
		{
			// std::cerr << "Usage:\tposture_error_vis <HTR> <PNG_grayscale>" << std::endl;
			const char* c_exts[] = { ".htr", ".png" };
			fs::path path_src(argv[1]);
			fs::path path_dst(argv[2]);
			std::string exts_input[] = { Norm(path_src.extension().u8string()), Norm(path_dst.extension().u8string()) };
			bool htr2png = (exts_input[0] == c_exts[0] && exts_input[1] == c_exts[1]);
			if (htr2png)
			{
				const char* path_htr = argv[1];
				const char* path_png = argv[2];
				HBVH htr = load_bvh_c(path_htr);
				unsigned int n_frames = get_n_frames(htr);
				cv::Mat err_out(n_frames, n_frames, CV_16U);
				HERRS err = alloc_err_table(htr);
				for (int i_frame = 0; i_frame < n_frames; i_frame++)
				{
					for (int j_frame = 0; j_frame < n_frames; j_frame++)
					{
						auto& vis_scale_ij = err_out.at<unsigned short>(i_frame, j_frame);
						vis_scale_ij = (unsigned short)(get_err(err, i_frame, j_frame) * USHRT_MAX);
					}
				}
				free_err_table(err);
				unload_bvh(htr);
				imwrite(path_png, err_out);
			}
			else
			{
				std::cout << "Not the right file extensions!!!" << std::endl;
			}
			
		}
		catch (std::string &info)
		{
			std::cout << "ERROR: " << info << std::endl;
		}
	}



	return 0;
}
