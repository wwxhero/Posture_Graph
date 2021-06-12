#include <iostream>
#include <bvh11.hpp>
#include <stack>
#include <queue>
#include "articulated_body.h"
#include "motion_pipeline.h"

typedef std::shared_ptr<const bvh11::Joint> Joint_bvh_ptr;
typedef std::pair<Joint_bvh_ptr, HBODY> Bound;

HBODY create_arti_body(bvh11::BvhObject& bvh, Joint_bvh_ptr j_bvh, int frame)
{
	auto name = j_bvh->name().c_str();
	auto tm_bvh = bvh.GetTransformationRelativeToParent(j_bvh, frame);
	Eigen::Quaterniond rq(tm_bvh.linear());
	Eigen::Vector3d tt = tm_bvh.translation();
	_TRANSFORM tm_hik = {
		{1, 1, 1},
		{rq.w(), rq.x(), rq.y(), rq.z()},
		{tt.x(), tt.y(), tt.z()}
	};
	auto b_hik = create_tree_body_node_c(name, &tm_hik);
	return b_hik;
}


HBODY create_arti_body_as_rest_bvh_pose(bvh11::BvhObject& bvh, Joint_bvh_ptr j_bvh, int frame)
{
	auto name = j_bvh->name().c_str();
	auto tm_bvh = bvh.GetTransformation(j_bvh, frame);
	Eigen::Affine3d tm_parent_bvh;
	auto j_parent_bvh = j_bvh->parent();
	if (nullptr == j_parent_bvh)
	{
		tm_parent_bvh.linear() = Eigen::Matrix3d::Identity();
		tm_parent_bvh.translation() = tm_bvh.translation();
	}
	else
	{
		tm_parent_bvh = bvh.GetTransformation(j_parent_bvh, frame);
	}

	Eigen::Vector3d tt = tm_bvh.translation() - tm_parent_bvh.translation();
	_TRANSFORM tm_hik = {
		{1, 1, 1},
		{1, 0, 0, 0},
		{tt.x(), tt.y(), tt.z()}
	};
	auto b_hik = create_tree_body_node_c(name, &tm_hik);
	return b_hik;
}


HBODY createArticulatedBody(bvh11::BvhObject& bvh, int frame, bool asRestBvhPose)
{
	//traverse the bvh herachical structure
	//	to create an articulated body with the given posture
	bool reset_posture = (frame < 0);
	std::queue<Bound> queBFS;
	auto root_j_bvh = bvh.root_joint();
	auto root_b_hik = asRestBvhPose
						? create_arti_body_as_rest_bvh_pose(bvh, root_j_bvh, frame)
						: create_arti_body(bvh, root_j_bvh, frame);
	Bound root = std::make_pair(
			root_j_bvh,
			root_b_hik
		);
	queBFS.push(root);
	while (!queBFS.empty())
	{
		auto pair = queBFS.front();
		auto j_bvh = pair.first;
		auto b_hik = pair.second;
		CNN cnn = FIRSTCHD;
		auto& rb_this = b_hik;
		for (auto j_bvh_child: j_bvh->children())
		{
			auto b_hik_child = asRestBvhPose
								? create_arti_body_as_rest_bvh_pose(bvh, j_bvh_child, frame)
								: create_arti_body(bvh, j_bvh_child, frame);
			Bound child = std::make_pair(
					j_bvh_child,
					b_hik_child
				);
			queBFS.push(child);
			auto& rb_next = b_hik_child;
			cnn_arti_body(rb_this, rb_next, cnn);
			cnn = NEXTSIB;
			rb_this = rb_next;
		}
		queBFS.pop();
	}
	return root_b_hik;
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

inline void printArtName(const char* name, int n_indent)
{
	std::string item;
	for (int i_indent = 0
		; i_indent < n_indent
		; i_indent ++)
		item += "\t";
	item += name;
	std::cout << item.c_str() << std::endl;
}

inline void printBoundName(Bound bnd, int n_indent)
{
	std::string item;
	for (int i_indent = 0
		; i_indent < n_indent
		; i_indent ++)
		item += "\t";
	std::string itemBvh(item);
	itemBvh += bnd.first->name();
	std::string itemBody(item);
	itemBody += body_name_c(bnd.second);
	std::cout << itemBvh.c_str() << std::endl;
	std::cout << itemBody.c_str() << std::endl;
}

inline bool BoundEQ(Bound bnd, const bvh11::BvhObject& bvh, int frame)
{
	const double epsilon = 1e-5;
	auto& joint_bvh = bnd.first;
	auto& body_hik = bnd.second;
	auto& name_bvh = joint_bvh->name();
	auto name_body = body_name_c(body_hik);
	bool name_eq = (name_bvh == name_body);
	Eigen::Affine3d tm_bvh = bvh.GetTransformation(joint_bvh, frame);
	Eigen::Matrix3d linear_tm_bvh = tm_bvh.linear();
	Eigen::Vector3d tt_tm_bvh = tm_bvh.translation();

	_TRANSFORM tm_hik;
	get_joint_transform_l2w(body_hik, &tm_hik);
	Eigen::Vector3r s(tm_hik.s.x, tm_hik.s.y, tm_hik.s.z);
	Eigen::Matrix3r r(Eigen::Quaternionr(tm_hik.r.w, tm_hik.r.x, tm_hik.r.y, tm_hik.r.z));
	Eigen::Matrix3r linear_tm_hik = r * s.asDiagonal();
	Eigen::Vector3r tt_tm_hik(tm_hik.tt.x, tm_hik.tt.y, tm_hik.tt.z);

	Eigen::Matrix3d diff_linear = linear_tm_bvh.inverse() * linear_tm_bvh;
	double abs_diff_linear = (diff_linear - Eigen::Matrix3d::Identity()).norm();
	bool linear_eq = (-epsilon < abs_diff_linear
						&& abs_diff_linear < epsilon);
	double diff_tt = (tt_tm_bvh - tt_tm_hik).norm();
	bool tt_eq = (-epsilon < diff_tt
						&& diff_tt < epsilon);

	return name_eq && linear_eq && tt_eq;
}

inline bool BoundResetAsBVHRest(Bound bnd, const bvh11::BvhObject& bvh, int frame)
{
	const double epsilon = 1e-5;
	auto& joint_bvh = bnd.first;
	auto& body_hik = bnd.second;
	auto& name_bvh = joint_bvh->name();
	auto name_body = body_name_c(body_hik);
	bool name_eq = (name_bvh == name_body);
	Eigen::Affine3d tm_bvh = bvh.GetTransformation(joint_bvh, frame);
	auto& joint_parent_bvh = joint_bvh->parent();
	Eigen::Affine3d tm_parent_bvh;
	if (nullptr == joint_parent_bvh)
	{
		tm_parent_bvh.linear() = Eigen::Matrix3d::Identity();
		tm_parent_bvh.translation() = tm_bvh.translation();
	}
	else
		tm_parent_bvh = bvh.GetTransformation(joint_parent_bvh, frame);
	Eigen::Vector3d tt_tm_bvh_l = tm_bvh.translation() - tm_parent_bvh.translation();


	_TRANSFORM tm_hik_l;
	get_joint_transform_l2p(body_hik, &tm_hik_l);
	Eigen::Vector3r s(tm_hik_l.s.x, tm_hik_l.s.y, tm_hik_l.s.z);
	Eigen::Vector3r tt_tm_hik_l(tm_hik_l.tt.x, tm_hik_l.tt.y, tm_hik_l.tt.z);

	Real diff_s = (s - Eigen::Vector3r::Ones()).norm();
	Real diff_r = Eigen::Vector4r(tm_hik_l.r.w - 1, tm_hik_l.r.x, tm_hik_l.r.y, tm_hik_l.r.z).norm();
	Real diff_tt = (tt_tm_bvh_l - tt_tm_hik_l).norm();
	bool linear_id = (-epsilon < diff_s
							&&	diff_s < epsilon
					&&-epsilon < diff_r
							&&  diff_r < epsilon);
	bool tt_eq = (-epsilon < diff_tt
					&& diff_tt < epsilon);

	return name_eq && linear_id && tt_eq;
}

template<typename LAMaccessEnter, typename LAMaccessLeave>
inline void TraverseDFS_botree_nonrecur(HBODY root, LAMaccessEnter OnEnterBody, LAMaccessLeave OnLeaveBody)
{
	assert(H_INVALID != root);
	typedef struct _EDGE
	{
		HBODY body_this;
		HBODY body_child;
	} EDGE;
	std::stack<EDGE> stkDFS;
	stkDFS.push({root, get_first_child_body(root)});
	//printArtName(body_name_w(root), 0);
	OnEnterBody(root);
	while (!stkDFS.empty())
	{
		EDGE &edge = stkDFS.top();
		size_t n_indent = stkDFS.size();
		if (H_INVALID == edge.body_child)
		{
			stkDFS.pop();
			OnLeaveBody(edge.body_this);
		}
		else
		{
			//printArtName(body_name_w(edge.body_child), n_indent);
			OnEnterBody(edge.body_child);
			HBODY body_grandchild = get_first_child_body(edge.body_child);
			HBODY body_nextchild = get_next_sibling_body(edge.body_child);
			stkDFS.push({edge.body_child, body_grandchild});
			edge.body_child = body_nextchild;
		}
	}
}

template<typename LAMaccessEnter, typename LAMaccessLeave>
inline void TraverseDFS_motree_nonrecur(HMOTIONNODE root, LAMaccessEnter OnEnterBody, LAMaccessLeave OnLeaveBody)
{
	assert(H_INVALID != root);
	typedef struct _EDGE
	{
		HMOTIONNODE body_this;
		HMOTIONNODE body_child;
	} EDGE;
	std::stack<EDGE> stkDFS;
	stkDFS.push({root, get_first_child_mo_node(root)});
	//printArtName(body_name_w(root), 0);
	OnEnterBody(root);
	while (!stkDFS.empty())
	{
		EDGE &edge = stkDFS.top();
		size_t n_indent = stkDFS.size();
		if (H_INVALID == edge.body_child)
		{
			stkDFS.pop();
			OnLeaveBody(edge.body_this);
		}
		else
		{
			//printArtName(body_name_w(edge.body_child), n_indent);
			OnEnterBody(edge.body_child);
			HMOTIONNODE body_grandchild = get_first_child_mo_node(edge.body_child);
			HMOTIONNODE body_nextchild = get_next_sibling_mo_node(edge.body_child);
			stkDFS.push({edge.body_child, body_grandchild});
			edge.body_child = body_nextchild;
		}
	}
}


template<typename LAMaccessEnter, typename LAMaccessLeave>
inline void TraverseDFS_boundtree_recur(Bound bound_this, LAMaccessEnter OnEnterBound, LAMaccessLeave OnLeaveBound)
{
	OnEnterBound(bound_this);
	auto bvh_this = bound_this.first;
	auto body_this = bound_this.second;
	const auto& children_bvh_this = bvh_this->children();
	auto it_bvh_next = children_bvh_this.begin();
	auto body_next = get_first_child_body(body_this);
	bool proceed = (it_bvh_next != children_bvh_this.end());
	assert((proceed)
		== (H_INVALID != body_next));
	while (proceed)
	{
		Bound bound_next = std::make_pair(*it_bvh_next, body_next);
		TraverseDFS_boundtree_recur(bound_next, OnEnterBound, OnLeaveBound);
		it_bvh_next++;
		body_next = get_next_sibling_body(body_next);
		proceed = (it_bvh_next != children_bvh_this.end());
		assert((proceed)
			== (H_INVALID != body_next));
	}
	OnLeaveBound(bound_this);
}

void updateHeader(bvh11::BvhObject& bvh, HBODY body)
{
	//update the header part of the BVH file
	//	, the given body is in BVH rest posture
	auto lam_onEnter = [&bvh = std::as_const(bvh)] (Bound b_this)
							{
								Joint_bvh_ptr joint_this = b_this.first;
								HBODY body_this = b_this.second;
								_TRANSFORM tm_l2p;
								get_joint_transform_l2p(body_this, &tm_l2p);
								const_cast<Eigen::Vector3d&>(joint_this->offset()) = Eigen::Vector3r(tm_l2p.tt.x, tm_l2p.tt.y, tm_l2p.tt.z);
							};
	auto lam_onLeave = [] (Bound b_this)
							{
							};
	Bound root = std::make_pair(bvh.root_joint(), body);
	TraverseDFS_boundtree_recur(root, lam_onEnter, lam_onLeave);
}

bool ResetRestPose(bvh11::BvhObject& bvh, int t)
{
	int n_frames = bvh.frames();
	bool in_range = (-1 < t
						&& t < n_frames);
	if (!in_range)
		return false;
	HBODY h_driver = createArticulatedBody(bvh, -1, false); //t = -1: the rest posture in BVH file
#if defined _DEBUG
	{
		std::cout << "Bounds:" << std::endl;
		int n_indent = 1;
		auto lam_onEnter = [&n_indent, &bvh = std::as_const(bvh)] (Bound b_this)
							{
								printBoundName(b_this, n_indent++);
								assert(BoundEQ(b_this, bvh, -1));
							};
		auto lam_onLeave = [&n_indent] (Bound b_this)
							{
								n_indent --;
							};
		Bound root = std::make_pair(bvh.root_joint(), h_driver);
		TraverseDFS_boundtree_recur(root, lam_onEnter, lam_onLeave);
	}
#endif
	HBODY h_driveeProxy = createArticulatedBody(bvh, t, false);
#if defined _DEBUG
	{
		std::cout << "Bounds:" << std::endl;
		int n_indent = 1;
		auto lam_onEnter = [&n_indent, &bvh = std::as_const(bvh), t] (Bound b_this)
							{
								printBoundName(b_this, n_indent++);
								assert(BoundEQ(b_this, bvh, t));
							};
		auto lam_onLeave = [&n_indent] (Bound b_this)
							{
								n_indent --;
							};
		Bound root = std::make_pair(bvh.root_joint(), h_driveeProxy);
		TraverseDFS_boundtree_recur(root, lam_onEnter, lam_onLeave);
	}
#endif
	HBODY h_drivee = createArticulatedBody(bvh, t, true);
#if defined _DEBUG
	{
		std::cout << "Bounds:" << std::endl;
		int n_indent = 1;
		auto lam_onEnter = [&n_indent, &bvh = std::as_const(bvh), t] (Bound b_this)
							{
								printBoundName(b_this, n_indent++);
								assert(BoundResetAsBVHRest(b_this, bvh, t));
							};
		auto lam_onLeave = [&n_indent] (Bound b_this)
							{
								n_indent --;
							};
		Bound root = std::make_pair(bvh.root_joint(), h_drivee);
		TraverseDFS_boundtree_recur(root, lam_onEnter, lam_onLeave);
	}
#endif
	updateHeader(bvh, h_drivee);

	HMOTIONNODE h_motion_driver = create_tree_motion_node(h_driver);
	HMOTIONNODE h_motion_driveeProxy = create_tree_motion_node(h_driveeProxy);
	motion_sync_cnn_homo(h_motion_driver, h_motion_driveeProxy);

	HMOTIONNODE h_motion_drivee = create_tree_motion_node(h_drivee);
	motion_sync_cnn_cross(h_motion_driveeProxy, h_motion_drivee, NULL, 0);


	for (int i_frame = 0
		; i_frame < n_frames
		; i_frame ++)
	{
		pose(h_driver, bvh, i_frame);
		motion_sync(h_motion_driver);
		updateAnim(h_drivee, bvh);
	}

	{
		auto lam_onEnter = [] (HMOTIONNODE node)
								{
								};
		auto lam_onLeave = [] (HMOTIONNODE node)
								{
									destroy_tree_motion_node(node);
								};
		// TraverseDFS_motree_nonrecur(h_motion_driver, lam_onEnter, lam_onLeave);
	}

	{
		auto lam_onEnter = [] (HBODY node)
								{
								};
		auto lam_onLeave = [] (HBODY node)
								{
									destroy_tree_body_node(node);
								};
		TraverseDFS_botree_nonrecur(h_driver, lam_onEnter, lam_onLeave);
	}
	return true;
}

int main(int argc, char* argv[])
{
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
