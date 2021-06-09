#include <iostream>
#include <bvh11.hpp>
#include "articulated_body.h"

HBODY createArticulatedBody(bvh11::BvhObject& bvh, int frame)
{
	//traverse the bvh herachical structure
	//	to create an articulated body with the given posture
	return H_INVALID;
}

HBODY createArticulatedBodyAsRestPose(bvh11::BvhObject& bvh, int frame)
{
	//traverse the bvh herachical structure
	//	to create an articulated body with the posture given by the frame No.
	//	as rest posture in BVH convention (I, offset)
	return H_INVALID;
}

void updateHeader(bvh11::BvhObject& bvh, HBODY body)
{
	//update the header part of the BVH file
	//	, the given body is in BVH rest posture
}

typedef void* HMOTIONPIPE;

HMOTIONPIPE createHomoSpaceMotionPipe(HBODY start, HBODY end)
{
	//the articulated bodies are in same coordinate space but in different postures
	return H_INVALID;
}

HMOTIONPIPE createXSpaceMotionPipe(HBODY start, HBODY end)
{
	//the articulated bodies are in different coordiante space but in same (aligned) posture
	return H_INVALID;
}

typedef void* HPIPELINE;

HPIPELINE createPipeline()
{
	//a pipe line contains several motion pipes
	return H_INVALID;
}

bool appendPipe(HPIPELINE line, HMOTIONPIPE pipe)
{
	//p == rear(line) -> end(p) == start(pipe)
	return false;
}

void pose(HBODY body, const bvh11::BvhObject& bvh, int i_frame)
{
	//pose the articulated body with the posture for frame i_frame
	//	the articulated body should have same rest posture as bvh
}

void executePipeLine(HPIPELINE line)
{
	// execute the pipe in sequence
}

void updateAnim(HBODY body, bvh11::BvhObject& bvh)
{
	// copy the joint (delta) transformation into BVH animation stack
}

void destroyPipeline(HPIPELINE line)
{

}

void destroyArticulatedBody(HBODY body)
{

}

void destroyMotionPipe(HMOTIONPIPE pipe)
{

}


bool ResetRestPose(bvh11::BvhObject& bvh, int t)
{
	int n_frames = bvh.frames();
	bool in_range = (-1 < t
						&& t < n_frames);
	if (!in_range)
		return false;

	HBODY h_driver = createArticulatedBody(bvh, -1); //t = -1: the rest posture in BVH file
	HBODY h_driveeProxy = createArticulatedBody(bvh, t);
	HBODY h_drivee = createArticulatedBodyAsRestPose(bvh, t);
	updateHeader(bvh, h_drivee);
	HMOTIONPIPE h_pipe_0 = createHomoSpaceMotionPipe(h_driver, h_driveeProxy);
	HMOTIONPIPE h_pipe_1 = createXSpaceMotionPipe(h_driveeProxy, h_drivee);
	HPIPELINE h_pipe_line = createPipeline();
	appendPipe(h_pipe_line, h_pipe_0);
	appendPipe(h_pipe_line, h_pipe_1);
	for (int i_frame = 0
		; i_frame < n_frames
		; i_frame ++)
	{
		pose(h_driver, bvh, i_frame);
		executePipeLine(h_pipe_line);
		updateAnim(h_drivee, bvh);
	}

	destroyPipeline(h_pipe_line);
	destroyMotionPipe(h_pipe_1);
	destroyMotionPipe(h_pipe_0);
	destroyArticulatedBody(h_driver);
	destroyArticulatedBody(h_driveeProxy);
	destroyArticulatedBody(h_drivee);

	return true;
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
