#ifndef BVH11_HPP_
#define BVH11_HPP_

#include <vector>
#include <list>
#include <string>
#include <memory>
#include <iostream>
#include <Eigen/Core>
#include <Eigen/Geometry>

namespace bvh11
{
    class Channel;
    class Joint;
    
    class BvhObject
    {
    public:
        BvhObject(const std::string& file_path)
        {
            ReadBvhFile(file_path);
        }
        
        int    frames()     const { return frames_;     }
        double frame_time() const { return frame_time_; }
        
        const std::vector<Channel>& channels() const { return channels_; }
        const Eigen::MatrixXd&      motion()   const { return motion_;   }
        
        std::shared_ptr<const Joint> root_joint() const { return root_joint_; }
        
        /// This method returns a list of joints always in the same order
        std::vector<std::shared_ptr<const Joint>> GetJointList() const;
        
        /// \param frame Frame. This value must be between 0 and frames() - 1.
        Eigen::Affine3d GetTransformationRelativeToParent(std::shared_ptr<const Joint> joint, int frame) const;
        
        /// \param frame Frame. This value must be between 0 and frames() - 1.
        Eigen::Affine3d GetTransformation(std::shared_ptr<const Joint> joint, int frame) const;
        
        /// \param frame Frame. This value must be between 0 and frames() - 1.
        Eigen::Affine3d GetRootTransformation(int frame) const
        {
            return GetTransformation(root_joint_, frame);
        }
        
        void PrintJointHierarchy() const { PrintJointSubHierarchy(root_joint_, 0); }
        
        void WriteBvhFile(const std::string& file_path) const;
        
    private:
        int                  frames_;
        double               frame_time_;
        
        std::vector<Channel> channels_;
        Eigen::MatrixXd      motion_;
        
        std::shared_ptr<const Joint> root_joint_;
        
        void ReadBvhFile(const std::string& file_path);
        
        void PrintJointSubHierarchy(std::shared_ptr<const Joint> joint, int depth) const;
        
        void WriteJointSubHierarchy(std::ofstream& ofs, std::shared_ptr<const Joint> joint, int depth) const;
    };
    
    class Channel
    {
    public:
        enum class Type
        {
            x_position, y_position, z_position,
            z_rotation, x_rotation, y_rotation
        };
        
        Channel(Type type, std::shared_ptr<Joint> target_joint) : type_(type), target_joint_(target_joint) {}
        
        const Type& type() const { return type_; }
        std::shared_ptr<Joint> target_joint() const { return target_joint_; }
        
    private:
        const Type type_;
        const std::shared_ptr<Joint> target_joint_;
    };
    
    std::ostream& operator<<(std::ostream& os, const Channel::Type& type);
    
    class Joint
    {
    public:
        Joint(const std::string& name, std::shared_ptr<Joint> parent) : name_(name), parent_(parent) {}
        
        const Eigen::Vector3d& offset() const { return offset_; }
        Eigen::Vector3d& offset() { return offset_; }
        
        bool has_end_site() const { return has_end_site_; }
        bool& has_end_site() { return has_end_site_; }
        
        const Eigen::Vector3d& end_site() const { return end_site_; }
        Eigen::Vector3d& end_site() { return end_site_; }
        
        const std::string& name() const { return name_; }
        
        const std::list<std::shared_ptr<Joint>>& children() const { return children_; }
        const std::list<int>& associated_channels_indices() const { return associated_channels_indices_; }
        
        std::shared_ptr<Joint> parent() const { return parent_; }
        
        void AddChild(std::shared_ptr<Joint> child) { children_.push_back(child); }
        void AssociateChannel(int channel_index) { associated_channels_indices_.push_back(channel_index); }
        
    private:
        const std::string            name_;
        const std::shared_ptr<Joint> parent_;
        
        bool                              has_end_site_ = false;
        Eigen::Vector3d                   end_site_;
        Eigen::Vector3d                   offset_;
        std::list<std::shared_ptr<Joint>> children_;
        std::list<int>                    associated_channels_indices_;
    };
}

#endif
