#include <iostream>
#include <bvh11.hpp>
#include <stack>
#include "articulated_body.h"
#include "motion_pipeline.h"

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




void pose(HBODY body, const bvh11::BvhObject& bvh, int i_frame)
{
	//pose the articulated body with the posture for frame i_frame
	//	the articulated body should have same rest posture as bvh
}



void updateAnim(HBODY body, bvh11::BvhObject& bvh)
{
	// copy the joint (delta) transformation into BVH animation stack
}



void destroyArticulatedBody(HBODY body)
{

}

template<typename CHARTYPE>
inline void printArtName(const CHARTYPE* name, int n_indent)
{
	std::basic_string<CHARTYPE> item;
	for (int i_indent = 0
		; i_indent < n_indent
		; i_indent ++)
		item += CHARTYPE("\t");
	item += name;
	std::cout << item.c_str() << std::endl;
}

template<typename LAMaccessEnter, typename LAMaccessLeave>
inline void TraverseDFS(HBODY root, LAMaccessEnter OnEnterBody, LAMaccessLeave OnLeaveBody)
{
	assert(H_INVALID != root);
	typedef struct _EDGE
	{
		HBODY body_this;
		HBODY body_child;
	} EDGE;
	std::stack<EDGE> stkDFS;
	stkDFS.push({root, get_first_child(root)});
	//printArtName(body_name_w(root), 0);
	OnEnterBody(root);
	while (!stkDFS.empty())
	{
		EDGE &edge = stkDFS.top();
		int n_indent = stkDFS.size();
		if (H_INVALID == edge.body_child)
		{
			stkDFS.pop();
			OnLeaveBody(edge.body_this);
		}
		else
		{
			//printArtName(body_name_w(edge.body_child), n_indent);
			OnEnterBody(edge.body_child);
			HBODY body_grandchild = get_first_child(edge.body_child);
			HBODY body_nextchild = get_next_sibling(edge.body_child);
			stkDFS.push({edge.body_child, body_grandchild});
			edge.body_child = body_nextchild;
		}
	}
}


bool ResetRestPose(bvh11::BvhObject& bvh, int t)
{
	int n_frames = bvh.frames();
	bool in_range = (-1 < t
						&& t < n_frames);
	if (!in_range)
		return false;
#if defined _DEBUG
	std::cout << "BVH joints:" << std::endl;
	bvh.PrintJointHierarchy();
#endif
	HBODY h_driver = createArticulatedBody(bvh, -1); //t = -1: the rest posture in BVH file
#if defined _DEBUG
	std::cout << "Articulated bodies:" << std::endl;
	int n_indent = 1;
	auto lam_onEnter = [&n_indent] (HBODY h_this)
						{
							printArtName(body_name_w(h_this), n_indent++);
						};
	auto lam_onLeave = [&n_indent] (HBODY h_this)
						{
							n_indent --;
						};
	TraverseDFS(h_driver, lam_onEnter, lam_onLeave);
#endif
	HBODY h_driveeProxy = createArticulatedBody(bvh, t);
	HBODY h_drivee = createArticulatedBodyAsRestPose(bvh, t);
	updateHeader(bvh, h_drivee);
	HMOTIONPIPE h_pipe_0 = createHomoSpaceMotionPipe(h_driver, h_driveeProxy);
	HMOTIONPIPE h_pipe_1 = createXSpaceMotionPipe(h_driveeProxy, h_drivee);
	HPIPELINE h_pipe_line = createPipeline(h_pipe_0);
	appendPipe(h_pipe_line, h_pipe_0, h_pipe_1);
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
