// sdk_command_node.hpp
#ifndef SDK_COMMAND__SDK_COMMAND_NODE_HPP_
#define SDK_COMMAND__SDK_COMMAND_NODE_HPP_

#include <chrono>
#include <memory>
#include <atomic>
#include <string>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "rcl_interfaces/msg/set_parameters_result.hpp"
#include "geometry_msgs/msg/pose_stamped.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "std_msgs/msg/string.hpp"
#include "ddt_msgs/msg/user_command.hpp"   // 新增 UserCommand 消息
#include "client.h"

class SDKCmdNode : public rclcpp::Node
{
public:
  explicit SDKCmdNode(const rclcpp::NodeOptions & options);

private:
  void commandCallback(const UDPCmdVel::CmdDataStruct& cmd);
  void errorCallback(const std::string& error_msg);
  void idleCheckCallback();
  void cmdKeyCallback(const std_msgs::msg::String::SharedPtr msg);  // 状态机切换回调  

  // 辅助函数：发布 UserCommand 消息（避免重复代码）
  void publishUserCommand(double lin_x, double lin_y, double lin_z,
                          double ang_x, double ang_y, double ang_z,
                          const std::string& fsm_mode);
  
  // ROS2 发布器（仅保留一个）
  rclcpp::Publisher<ddt_msgs::msg::UserCommand>::SharedPtr user_cmd_publisher_;
  
  // 状态机切换订阅器（保留，用于接收外部切换命令）
  rclcpp::Subscription<std_msgs::msg::String>::SharedPtr cmd_key_sub_;
  rclcpp::TimerBase::SharedPtr idle_check_timer_;
  rclcpp::Time last_cmd_time_;

  // UDP接收器相关
  std::unique_ptr<UDPCmdVel::Receiver> receiver_;
  
  // 配置参数
  std::string server_ip_;
  int server_port_;
  int client_port_;
  std::string initial_fsm_mode_;  // 初始状态机模式（从参数文件读取）
  std::string current_fsm_mode_;  // 当前状态机模式（用于判断是否在 loco 状态）
  
  // 姿态控制参数（仅在 loco 状态下有效）
  std::string pose_frame_id_ = "world";  // frame_id，默认 "world"
  double pose_position_z_ = 0.2;         // 位置 z (米)，范围 0.1 到 0.3
  double pose_orientation_x_ = 0.0;      // 四元数 x
  double pose_orientation_y_ = 0.171;    // 四元数 y
  double pose_orientation_z_ = 0.0;      // 四元数 z
  double pose_orientation_w_ = 0.985;    // 四元数 w
  
  // 坐标系参数（将用于 UserCommand 的 header.frame_id）
  std::string twist_frame_id_ = "base_link";
  
  // 参数变更回调相关
  rclcpp::node_interfaces::OnSetParametersCallbackHandle::SharedPtr param_callback_handle_;
  
  // 辅助函数
  bool isValidFsmMode(const std::string& mode) const;
  
  // 参数变更回调函数
  rcl_interfaces::msg::SetParametersResult onSetParameters(
    const std::vector<rclcpp::Parameter> &parameters);
};

#endif  // SDK_COMMAND__SDK_COMMAND_NODE_HPP_