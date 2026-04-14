// sdk_command_node.cpp
#include "sdk_command/sdk_command_node.hpp"
#include <cstdlib>
#include <algorithm>
#include <memory>

SDKCmdNode::SDKCmdNode(const rclcpp::NodeOptions & options) : Node("user_command_node", options)
{
  // 获取参数
  this->get_parameter_or<std::string>("server_ip", server_ip_, "192.168.19.101");
  this->get_parameter_or<int>("server_port", server_port_, 8888);
  this->get_parameter_or<int>("client_port", client_port_, 9999);
  this->get_parameter_or<std::string>("fsm_mode", initial_fsm_mode_, "idle");
  
  // 姿态控制参数（仅在 loco 状态下有效）
  this->get_parameter_or<std::string>("pose_frame_id", pose_frame_id_, "world");
  this->get_parameter_or<double>("pose_position_z", pose_position_z_, 0.2);
  this->get_parameter_or<double>("pose_orientation_x", pose_orientation_x_, 0.0);
  this->get_parameter_or<double>("pose_orientation_y", pose_orientation_y_, 0.171);
  this->get_parameter_or<double>("pose_orientation_z", pose_orientation_z_, 0.0);
  this->get_parameter_or<double>("pose_orientation_w", pose_orientation_w_, 0.985);
  
  this->get_parameter_or<std::string>("twist_frame_id", twist_frame_id_, "base_link");
  
  // 验证 pose_position_z 范围
  if (pose_position_z_ < 0.1 || pose_position_z_ > 0.3) {
    RCLCPP_WARN(this->get_logger(), 
                "pose_position_z (%.3f) 超出有效范围 [0.1, 0.3]，将限制在范围内", 
                pose_position_z_);
    pose_position_z_ = std::max(0.1, std::min(0.3, pose_position_z_));
  }
  
  current_fsm_mode_ = initial_fsm_mode_;
  
  // 获取机器人命名空间
  std::string robot_ns;
  const char* env_ns = std::getenv("ROBOT_NS");
  if (env_ns == nullptr) {
    RCLCPP_ERROR(this->get_logger(), "环境变量 ROBOT_NS 未设置！");
    robot_ns = "";
  } else {
    robot_ns = std::string(env_ns);
  }
  
  // 构建 UserCommand 话题名称
  std::string user_cmd_topic = "/" + robot_ns + "/command/user_command";
  
  RCLCPP_INFO(this->get_logger(), "User SDK with UDP receiver! Server IP: %s, Robot NS: %s", 
              server_ip_.c_str(), robot_ns.c_str());
  RCLCPP_INFO(this->get_logger(), "Publishing to topic: %s", user_cmd_topic.c_str());
  
  // 创建单一发布器
  user_cmd_publisher_ = this->create_publisher<ddt_msgs::msg::UserCommand>(user_cmd_topic, 10);
  
  // 注册参数变更回调
  param_callback_handle_ = this->add_on_set_parameters_callback(
    std::bind(&SDKCmdNode::onSetParameters, this, std::placeholders::_1));
  
  // 如果配置了初始状态机模式，立即发布一条初始命令（零速度，仅切换模式）
  if (!initial_fsm_mode_.empty() && isValidFsmMode(initial_fsm_mode_)) {
    publishUserCommand(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, initial_fsm_mode_);
    RCLCPP_INFO(this->get_logger(), "发布初始状态机模式: %s", initial_fsm_mode_.c_str());
  } else if (!initial_fsm_mode_.empty()) {
    RCLCPP_WARN(this->get_logger(), 
                "配置的初始状态机模式 '%s' 无效，将跳过。有效模式: transform_up, idle, transform_down, loco, joint_pd, car, rl_1, rl_2, rl_3",
                initial_fsm_mode_.c_str());
  }
  
  // 初始化 UDP 接收器
  receiver_ = std::make_unique<UDPCmdVel::Receiver>(server_ip_, server_port_, client_port_);
  idle_check_timer_ = this->create_wall_timer(
    std::chrono::milliseconds(50),
    std::bind(&SDKCmdNode::idleCheckCallback, this)
  );
  
  last_cmd_time_ = this->now();
  
  if (!receiver_->start(
      std::bind(&SDKCmdNode::commandCallback, this, std::placeholders::_1),
      std::bind(&SDKCmdNode::errorCallback, this, std::placeholders::_1))) {
    RCLCPP_ERROR(this->get_logger(), "Failed to start UDP receiver!");
  } else {
    RCLCPP_INFO(this->get_logger(), "UDP receiver started successfully.");
  }
}

void SDKCmdNode::publishUserCommand(double lin_x, double lin_y, double lin_z,
                                    double ang_x, double ang_y, double ang_z,
                                    const std::string& fsm_mode)
{
  auto cmd = std::make_unique<ddt_msgs::msg::UserCommand>();
  cmd->header.stamp = this->now();
  cmd->header.frame_id = twist_frame_id_;   // 使用配置的坐标系
  cmd->fsm_mode = fsm_mode;
  
  // 填充 twist
  cmd->twist.linear.x = lin_x;
  cmd->twist.linear.y = lin_y;
  cmd->twist.linear.z = lin_z;
  cmd->twist.angular.x = ang_x;
  cmd->twist.angular.y = ang_y;
  cmd->twist.angular.z = ang_z;
  
  // 填充 pose (根据 fsm_mode 决定)
  if (fsm_mode == "loco") {
    cmd->pose.position.x = 0.0;
    cmd->pose.position.y = 0.0;
    cmd->pose.position.z = pose_position_z_;
    cmd->pose.orientation.x = pose_orientation_x_;
    cmd->pose.orientation.y = pose_orientation_y_;
    cmd->pose.orientation.z = pose_orientation_z_;
    cmd->pose.orientation.w = pose_orientation_w_;
  } else {
    cmd->pose.position.x = 0.0;
    cmd->pose.position.y = 0.0;
    cmd->pose.position.z = 0.0;
    cmd->pose.orientation.x = 0.0;
    cmd->pose.orientation.y = 0.0;
    cmd->pose.orientation.z = 0.0;
    cmd->pose.orientation.w = 1.0;
  }
  
  if (user_cmd_publisher_) {
    user_cmd_publisher_->publish(std::move(cmd));
  }
}

void SDKCmdNode::commandCallback(const UDPCmdVel::CmdDataStruct& cmd)
{
  last_cmd_time_ = this->now();
  
  // 发布 UserCommand，使用当前的状态机模式
  publishUserCommand(cmd.linear_x, cmd.linear_y, cmd.linear_z,
                     cmd.angular_x, cmd.angular_y, cmd.angular_z,
                     current_fsm_mode_);
  
  RCLCPP_INFO(
    this->get_logger(),
    "RECV CMD -> lin[x=%.2f, y=%.2f, z=%.2f], ang[z=%.2f]",
    cmd.linear_x, cmd.linear_y, cmd.linear_z, cmd.angular_z
  );
}

void SDKCmdNode::errorCallback(const std::string& error_msg)
{
  RCLCPP_ERROR(this->get_logger(), "UDP recv error: %s", error_msg.c_str());
}

bool SDKCmdNode::isValidFsmMode(const std::string& mode) const
{
  const std::vector<std::string> valid_modes = {
    "transform_up", "idle", "transform_down", "loco", "joint_pd", "car", "rl_1", "rl_2", "rl_3"
  };
  return std::find(valid_modes.begin(), valid_modes.end(), mode) != valid_modes.end();
}

rcl_interfaces::msg::SetParametersResult SDKCmdNode::onSetParameters(
  const std::vector<rclcpp::Parameter> &parameters)
{
  rcl_interfaces::msg::SetParametersResult result;
  result.successful = true;
  
  for (const auto &param : parameters) {
    if (param.get_name() == "fsm_mode") {
      std::string new_mode = param.as_string();
      if (isValidFsmMode(new_mode)) {
        initial_fsm_mode_ = new_mode;
        current_fsm_mode_ = new_mode;
        // 立即发布一条零速度命令，携带新的 fsm_mode
        publishUserCommand(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, new_mode);
        RCLCPP_INFO(this->get_logger(), "通过参数修改状态机模式: %s", new_mode.c_str());
      } else {
        result.successful = false;
        result.reason = "无效的状态机模式: " + new_mode;
        RCLCPP_WARN(this->get_logger(), 
                    "参数修改失败：无效的状态机模式 '%s'。有效模式: transform_up, idle, transform_down, loco, joint_pd, car, rl_1, rl_2, rl_3",
                    new_mode.c_str());
      }
    } else if (param.get_name() == "twist_frame_id") {
      twist_frame_id_ = param.as_string();
      RCLCPP_INFO(this->get_logger(), "更新坐标系: %s", twist_frame_id_.c_str());
    }
  }
  return result;
}

void SDKCmdNode::cmdKeyCallback(const std_msgs::msg::String::SharedPtr msg)
{
  std::string mode = msg->data;
  if (!isValidFsmMode(mode)) {
    RCLCPP_WARN(this->get_logger(), 
                "收到无效的状态机模式: '%s'。有效模式: transform_up, idle, transform_down, loco, joint_pd, car, rl_1, rl_2, rl_3",
                mode.c_str());
    return;
  }
  
  initial_fsm_mode_ = mode;
  current_fsm_mode_ = mode;
  // 发布零速度命令，仅切换模式
  publishUserCommand(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, mode);
  RCLCPP_INFO(this->get_logger(), "通过键盘命令切换状态机模式: %s", mode.c_str());
}

void SDKCmdNode::idleCheckCallback()
{
  auto current_time = this->now();
  auto time_diff = (current_time - last_cmd_time_).seconds();
  
  if (time_diff > 0.2) {
    // 发布零速度命令，保持当前 fsm_mode
    publishUserCommand(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, current_fsm_mode_);
    
    RCLCPP_DEBUG_THROTTLE(
      this->get_logger(),
      *this->get_clock(),
      1000,
      "发布零命令（空闲超时 %.2f 秒）", time_diff
    );
  }
}