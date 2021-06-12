#include <bvh11.hpp>
#include <regex>
#include <cassert>
#include <fstream>
#include <queue>

namespace bvh11
{
	namespace internal
	{
		inline std::vector<std::string> split(const std::string& sequence, const std::string& pattern)
		{
			const std::regex regex(pattern);
			const std::sregex_token_iterator first{ sequence.begin(), sequence.end(), regex, -1 };
			const std::sregex_token_iterator last {};
			return std::vector<std::string>{ first->str().empty() ? std::next(first) : first, last };
		}

		inline std::vector<std::string> tokenize_next_line(std::ifstream& ifs)
		{
			std::string line;
			if (std::getline(ifs, line))
			{
				return internal::split(line, R"([\t\s]+)");
			}
			else
			{
				assert(false && "Failed to read the next line");
				return {};
			}
		}

		inline Eigen::Vector3d read_offset(const std::vector<std::string>& tokens)
		{
			assert(tokens.size() == 4 && tokens[0] == "OFFSET");
			const double offset_x = std::stod(tokens[1]);
			const double offset_y = std::stod(tokens[2]);
			const double offset_z = std::stod(tokens[3]);
			return Eigen::Vector3d(offset_x, offset_y, offset_z);
		}
	}

	std::vector<std::shared_ptr<const Joint>> BvhObject::GetJointList() const
	{
		std::vector<std::shared_ptr<const Joint>> joint_list;
		std::function<void(std::shared_ptr<const Joint>)> add_joint = [&](std::shared_ptr<const Joint> joint)
		{
			joint_list.push_back(joint);
			for (auto child : joint->children()) { add_joint(child); }
		};
		add_joint(root_joint_);
		return joint_list;
	}

	void BvhObject::UpdateMotion(std::shared_ptr<const Joint> joint, const Eigen::Affine3d& tm_delta_l, int i_frame)
	{
		Eigen::Index order2rot_i[3] = { 0 };
		int rot_i2order[3] = { 0 };
		int order = 0;
		auto & idx_jc = joint->associated_channels_indices();
		for (auto it_i_c = idx_jc.begin()
			; order < 3 && it_i_c != idx_jc.end()
			; it_i_c ++)
		{
			int i_channel = *it_i_c;
			const bvh11::Channel& channel = channels()[i_channel];
			bool is_a_rotation = ( bvh11::Channel::Type::x_rotation == channel.type
								|| bvh11::Channel::Type::y_rotation == channel.type
								|| bvh11::Channel::Type::z_rotation == channel.type);
			if (is_a_rotation)
			{
				Eigen::Index rot_i = -1;
				switch (channel.type)
				{
					case bvh11::Channel::Type::x_rotation:
						rot_i = 0;
						break;
					case bvh11::Channel::Type::y_rotation:
						rot_i = 1;
						break;
					case bvh11::Channel::Type::z_rotation:
						rot_i = 2;
						break;
				}

				order2rot_i[order] = rot_i;
				rot_i2order[rot_i] = order;
				order ++;
			}
		}
		assert(3 == order
			&& order2rot_i[0] != order2rot_i[1]
			&& order2rot_i[0] != order2rot_i[2]
			&& order2rot_i[1] != order2rot_i[2]
			&& "make sure the order2rot_i is correctly recording the order of rotation");
		Eigen::Vector3d rots_euler = tm_delta_l.rotation().eulerAngles(order2rot_i[0], order2rot_i[1], order2rot_i[2]);
		const auto& data_euler = rots_euler.data();
		double value_rots[] = {
								data_euler[rot_i2order[0]], // rotate about x
								data_euler[rot_i2order[1]], // rotate about y
								data_euler[rot_i2order[2]], // rotate about z
							};

		const double c_rad2deg = 180.0/M_PI;

		const Eigen::Vector3d& value_tt = tm_delta_l.translation();
		bool has_translation_channel = false;
		for (int channel_index : joint->associated_channels_indices())
		{
			const bvh11::Channel& channel = channels()[channel_index];
			double&          value = const_cast<double&>(motion()(i_frame, channel_index));

			switch (channel.type)
			{
			case bvh11::Channel::Type::x_position:
				value = value_tt.x();
				has_translation_channel = true;
				break;
			case bvh11::Channel::Type::y_position:
				value = value_tt.y();
				has_translation_channel = true;
				break;
			case bvh11::Channel::Type::z_position:
				value = value_tt.z();
				has_translation_channel = true;
				break;
			case bvh11::Channel::Type::x_rotation:
				value = value_rots[0] * c_rad2deg;
				break;
			case bvh11::Channel::Type::y_rotation:
				value = value_rots[1] * c_rad2deg;
				break;
			case bvh11::Channel::Type::z_rotation:
				value = value_rots[2] * c_rad2deg;
				break;
			}
		}
		double epsilon = 1e-3;
		bool has_translation_in_delta_tm = (value_tt.norm() > epsilon);
		assert((!has_translation_in_delta_tm || has_translation_channel)
			&& "has_translation_in_delta_tm -> has_translation_channel");
	}

	bool BvhObject::ResetRestPose(int nframe)
	{
		if (-1 < nframe
			  && nframe < frames())
		{
			const double c_epsilon = 1e-3;
			typedef std::shared_ptr<const Joint> Joint_ptr;
			auto joints = GetJointList();
			size_t n_joints = joints.size();
			std::vector<Eigen::Vector3d> offset_b(n_joints);
			std::vector<Eigen::Affine3d> bind_a2b(n_joints);
			std::vector<Eigen::Affine3d> bind_b2a(n_joints);
			std::vector<Eigen::Affine3d> bind_p2l_a(n_joints);

			for (int i_joint = 0; i_joint < n_joints; i_joint ++)
			{
				Joint_ptr joint = joints[i_joint];
				Eigen::Affine3d bind_l2w_i = GetTransformation(joint, nframe);
				Eigen::Affine3d bind_p2w_i;
				Joint_ptr joint_parent = joint->parent();
				if (nullptr == joint_parent)
				{
					bind_p2w_i.linear() = Eigen::Matrix3d::Identity();
					bind_p2w_i.translation() = bind_l2w_i.translation();
				}
				else
				{
					bind_p2w_i = GetTransformation(joint_parent, nframe);
				}

				Eigen::Vector3d offset_i = bind_l2w_i.translation() - bind_p2w_i.translation();
				Eigen::Affine3d bind_a2b_i(bind_l2w_i.linear());
				Eigen::Affine3d bind_b2a_i(bind_a2b_i.inverse());
				Eigen::Affine3d bind_p2l_i = bind_l2w_i.inverse() * bind_p2w_i;

				offset_b[i_joint] = offset_i;
				bind_a2b[i_joint] = bind_a2b_i;
				bind_b2a[i_joint] = bind_b2a_i;
				bind_p2l_a[i_joint] = bind_p2l_i;
			}

			std::vector<Eigen::Affine3d> delta_b_l;
			delta_b_l.resize(joints.size());
			for (int i_frame = 0; i_frame < frames_; i_frame++)
			{
				for (int i_joint = 0; i_joint < joints.size(); i_joint ++)
				{
					Joint_ptr joint = joints[i_joint];
					Eigen::Affine3d delta_l2p_a_i = GetTransformationRelativeToParent(joint, i_frame);
					Eigen::Affine3d delta_a_l_i = bind_p2l_a[i_joint]*delta_l2p_a_i;
					Eigen::Affine3d delta_b_l_i = bind_a2b[i_joint] * delta_a_l_i * bind_b2a[i_joint];
					delta_b_l[i_joint] = delta_b_l_i;
				}

				for (int i_joint = 0; i_joint < joints.size(); i_joint ++)
				{
					UpdateMotion(joints[i_joint], delta_b_l[i_joint], i_frame);
				}
			}

			for (int i_joint = 0; i_joint < n_joints; i_joint ++)
			{
				Joint_ptr joint = joints[i_joint];
				const_cast<Eigen::Vector3d&>(joint->offset()) = offset_b[i_joint];
			}

			return true;
		}
		else
			return false;
	}

	Eigen::Affine3d BvhObject::GetLocalDeltaTM(std::shared_ptr<const Joint> joint, int frame) const
	{
		bool has_a_rotation = false;
		bool has_a_translation = false;
		Eigen::Affine3d tm_delta_l = Eigen::Affine3d::Identity();
		for (int channel_index : joint->associated_channels_indices())
		{
			const bvh11::Channel& channel = channels()[channel_index];
			const double          value   = motion()(frame, channel_index);

			switch (channel.type)
			{
				case bvh11::Channel::Type::x_position:
					tm_delta_l *= Eigen::Translation3d(Eigen::Vector3d(value, 0.0, 0.0));
					has_a_translation = true;
					break;
				case bvh11::Channel::Type::y_position:
					tm_delta_l *= Eigen::Translation3d(Eigen::Vector3d(0.0, value, 0.0));
					has_a_translation = true;
					break;
				case bvh11::Channel::Type::z_position:
					tm_delta_l *= Eigen::Translation3d(Eigen::Vector3d(0.0, 0.0, value));
					has_a_translation = true;
					break;
				case bvh11::Channel::Type::x_rotation:
					tm_delta_l *= Eigen::AngleAxisd(value * M_PI / 180.0, Eigen::Vector3d::UnitX());
					has_a_rotation = true;
					break;
				case bvh11::Channel::Type::y_rotation:
					tm_delta_l *= Eigen::AngleAxisd(value * M_PI / 180.0, Eigen::Vector3d::UnitY());
					has_a_rotation = true;
					break;
				case bvh11::Channel::Type::z_rotation:
					tm_delta_l *= Eigen::AngleAxisd(value * M_PI / 180.0, Eigen::Vector3d::UnitZ());
					has_a_rotation = true;
					break;
			}
		}

		assert((joint->associated_channels_indices().size() != 3 || (has_a_rotation && !has_a_translation)) && "|channels| == 3 -> (rotation && not translation)"
			&& (joint->associated_channels_indices().size() != 6 || (has_a_rotation && has_a_translation)) && "|channels| == 6 -> (rotation && translation)");

		return tm_delta_l;
	}

	Eigen::Affine3d BvhObject::GetTransformationRelativeToParent(std::shared_ptr<const Joint> joint, int frame) const
	{
		assert(frame < frames() && "Invalid frame is specified.");
		assert(joint->associated_channels_indices().size() == 3 || joint->associated_channels_indices().size() == 6);

		Eigen::Affine3d tm_l2p_0(Eigen::Translation3d(joint->offset()));
		// having:
		//		tm_l2p_0.linear() == Eigen::Matrix3d::Identity();
		//		tm_l2p_0.translation() == joint->offset();
		bool rest_posture = (frame < 0);
		if (!rest_posture)
		{
			Eigen::Affine3d tm_delta_l = GetLocalDeltaTM(joint, frame);
			return tm_l2p_0 * tm_delta_l;
		}
		else
			return tm_l2p_0;
	}

	Eigen::Affine3d BvhObject::GetTransformation(std::shared_ptr<const Joint> joint, int frame) const
	{
		Eigen::Affine3d transform = GetTransformationRelativeToParent(joint, frame);

		while (joint->parent().get() != nullptr)
		{
			joint = joint->parent();

			transform = GetTransformationRelativeToParent(joint, frame) * transform;
		}

		return transform;
	}

	void BvhObject::ReadBvhFile(const std::string& file_path, const double scale)
	{
		// Open the input file
		std::ifstream ifs(file_path);
		assert(ifs.is_open() && "Failed to open the input file.");

		// Read the HIERARCHY part
		[&]() -> void
		{
			std::string line;
			std::vector<std::shared_ptr<Joint>> stack;
			while (std::getline(ifs, line))
			{
				// Split the line into tokens
				const std::vector<std::string> tokens = internal::split(line, R"([\t\s]+)");

				// Ignore empty lines
				if (tokens.empty())
				{
					continue;
				}
				// Ignore a declaration of hierarchy section
				else if (tokens[0] == "HIERARCHY")
				{
					continue;
				}
				// Start to create a new joint
				else if (tokens[0] == "ROOT" || tokens[0] == "JOINT")
				{
					assert(tokens.size() == 2 && "Failed to find a joint name");

					// Read the joint name
					const std::string& joint_name = tokens[1];

					// Get a pointer for the parent if this is not a root joint
					const std::shared_ptr<Joint> parent = stack.empty() ? nullptr : stack.back();

					// Instantiate a new joint
					std::shared_ptr<Joint> new_joint = std::make_shared<Joint>(joint_name, parent);

					// Register it to the parent's children list
					if (parent) { parent->AddChild(new_joint); }
					else { root_joint_ = new_joint; }

					// Add the new joint to the stack
					stack.push_back(new_joint);

					// Read the next line, which should be "{"
					const std::vector<std::string> tokens_begin_block = internal::tokenize_next_line(ifs);
					assert(tokens_begin_block.size() == 1 && "Found two or more tokens");
					assert(tokens_begin_block[0] == "{" && "Could not find an expected '{'");
				}
				// Read an offset value
				else if (tokens[0] == "OFFSET")
				{
					assert(tokens.size() == 4);
					const std::shared_ptr<Joint> current_joint = stack.back();
					current_joint->offset() = scale * internal::read_offset(tokens);
				}
				// Read a channel list
				else if (tokens[0] == "CHANNELS")
				{
					assert(tokens.size() >= 2);
					const int num_channels = std::stoi(tokens[1]);

					assert(tokens.size() == num_channels + 2);
					for (int i = 0; i < num_channels; ++ i)
					{
						const std::shared_ptr<Joint> target_joint = stack.back();
						const Channel::Type type = [](const std::string& channel_type)
						{
							if (channel_type == "Xposition") { return Channel::Type::x_position; }
							if (channel_type == "Yposition") { return Channel::Type::y_position; }
							if (channel_type == "Zposition") { return Channel::Type::z_position; }
							if (channel_type == "Zrotation") { return Channel::Type::z_rotation; }
							if (channel_type == "Xrotation") { return Channel::Type::x_rotation; }
							if (channel_type == "Yrotation") { return Channel::Type::y_rotation; }

							assert(false && "Could not find a valid channel type");
							return Channel::Type();
						}(tokens[i + 2]);

						channels_.push_back(Channel{ type, target_joint });

						const int channel_index = static_cast<int>(channels_.size() - 1);
						target_joint->AssociateChannel(channel_index);
					}
				}
				// Read an end site
				else if (tokens[0] == "End")
				{
					assert(tokens.size() == 2 && tokens[1] == "Site");

					const std::shared_ptr<Joint> current_joint = stack.back();
					current_joint->has_end_site() = true;

					// Read the next line, which should be "{"
					const std::vector<std::string> tokens_begin_block = internal::tokenize_next_line(ifs);
					assert(tokens_begin_block.size() == 1 && "Found two or more tokens");
					assert(tokens_begin_block[0] == "{" && "Could not find an expected '{'");

					// Read the next line, which should state an offset
					const std::vector<std::string> tokens_offset = internal::tokenize_next_line(ifs);
					current_joint->end_site() = scale * internal::read_offset(tokens_offset);

					// Read the next line, which should be "{"
					const std::vector<std::string> tokens_end_block = internal::tokenize_next_line(ifs);
					assert(tokens_end_block.size() == 1 && "Found two or more tokens");
					assert(tokens_end_block[0] == "}" && "Could not find an expected '}'");
				}
				// Finish to create a joint
				else if (tokens[0] == "}")
				{
					assert(!stack.empty());
					stack.pop_back();
				}
				// Stop this iteration and go to the motion section
				else if (tokens[0] == "MOTION")
				{
					return;
				}
			}
			assert(false && "Could not find the MOTION part");
		}();

		// Read the MOTION part
		[&]() -> void
		{
			// Read the number of frames
			const std::vector<std::string> tokens_frames = internal::tokenize_next_line(ifs);
			assert(tokens_frames.size() == 2);
			assert(tokens_frames[0] == "Frames:");
			frames_ = std::stoi(tokens_frames[1]);

			// Read the frame time
			const std::vector<std::string> tokens_frame_time = internal::tokenize_next_line(ifs);
			assert(tokens_frame_time.size() == 3);
			assert(tokens_frame_time[0] == "Frame" && tokens_frame_time[1] == "Time:");
			frame_time_ = std::stod(tokens_frame_time[2]);

			// Allocate memory for storing motion data
			motion_.resize(frames_, channels_.size());

			// Read each frame
			for (int frame_index = 0; frame_index < frames_; ++ frame_index)
			{
				const std::vector<std::string> tokens = internal::tokenize_next_line(ifs);
				assert(tokens.size() == channels_.size() && "Found invalid motion data");

				for (int channel_index = 0; channel_index < channels_.size(); ++ channel_index)
				{
					motion_(frame_index, channel_index) = std::stod(tokens[channel_index]);
				}
			}

			// Scale translations
			for (int channel_index = 0; channel_index < channels_.size(); ++ channel_index)
			{
				const Channel::Type& type = channels_[channel_index].type;
				if (type == Channel::Type::x_position || type == Channel::Type::y_position || type == Channel::Type::z_position)
				{
					motion_.col(channel_index) = scale * motion_.col(channel_index);
				}
			}
		}();
	}

	void BvhObject::PrintJointSubHierarchy(std::shared_ptr<const Joint> joint, int depth) const
	{
		for (int i = 0; i < depth; ++ i) { std::cout << "  "; }
		std::cout << joint->name() << std::endl;

		for (auto child : joint->children()) { PrintJointSubHierarchy(child, depth + 1); }
	}

	void BvhObject::WriteJointSubHierarchy(std::ofstream& ofs, std::shared_ptr<const Joint> joint, int depth) const
	{
		auto indent_creation = [](int depth) -> std::string
		{
			std::string tabs = "";
			for (int i = 0; i < depth; ++ i) { tabs += "\t"; }
			return tabs;
		};

		ofs << indent_creation(depth);
		ofs << (joint->parent() == nullptr ? "ROOT" : "JOINT");
		ofs << " " << joint->name() << "\n";

		ofs << indent_creation(depth);
		ofs << "{" << "\n";

		ofs << indent_creation(depth + 1);
		ofs << "OFFSET" << " ";
		ofs << joint->offset()(0) << " " << joint->offset()(1) << " " << joint->offset()(2);
		ofs << "\n";

		const auto associated_channels_indices = joint->associated_channels_indices();
		ofs << indent_creation(depth + 1);
		ofs << "CHANNELS" << " " << associated_channels_indices.size();
		for (auto i : associated_channels_indices)
		{
			ofs << " " << channels()[i].type;
		}
		ofs << "\n";

		if (joint->has_end_site())
		{
			ofs << indent_creation(depth + 1);
			ofs << "End Site" << "\n";
			ofs << indent_creation(depth + 1);
			ofs << "{" << "\n";
			ofs << indent_creation(depth + 2);
			ofs << "OFFSET" << " ";
			ofs << joint->end_site()(0) << " " << joint->end_site()(1) << " " << joint->end_site()(2);
			ofs << "\n";
			ofs << indent_creation(depth + 1);
			ofs << "}" << "\n";
		}

		for (auto child : joint->children())
		{
			WriteJointSubHierarchy(ofs, child, depth + 1);
		}

		ofs << indent_creation(depth);
		ofs << "}" << "\n";
	}

	void BvhObject::WriteBvhFile(const std::string& file_path) const
	{
		// Eigen format
		const Eigen::IOFormat motion_format(Eigen::StreamPrecision, Eigen::DontAlignCols, " ", "", "", "\n", "", "");

		// Open the input file
		std::ofstream ofs(file_path);
		assert(ofs.is_open() && "Failed to open the output file.");

		// Hierarch
		ofs << "HIERARCHY" << "\n";
		WriteJointSubHierarchy(ofs, root_joint_, 0);

		// Motion
		ofs << "MOTION" << "\n";
		ofs << "Frames: " << frames_ << "\n";
		ofs << "Frame Time: " << frame_time_ << "\n";
		ofs << motion_.format(motion_format);
	}

	void BvhObject::ResizeFrames(int num_new_frames)
	{
		assert(num_new_frames >= 0 && "Received an invalid frame number.");

		motion_.conservativeResize(num_new_frames, Eigen::NoChange);
		frames_ = num_new_frames;
		return;
	}

	std::ostream& operator<<(std::ostream& os, const Channel::Type& type)
	{
		switch (type) {
			case Channel::Type::x_position:
				os << "Xposition";
				break;
			case Channel::Type::y_position:
				os << "Yposition";
				break;
			case Channel::Type::z_position:
				os << "Zposition";
				break;
			case Channel::Type::x_rotation:
				os << "Xrotation";
				break;
			case Channel::Type::y_rotation:
				os << "Yrotation";
				break;
			case Channel::Type::z_rotation:
				os << "Zrotation";
				break;
		}
		return os;
	}
}
