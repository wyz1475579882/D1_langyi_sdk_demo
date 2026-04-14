#!/bin/bash
set -e
sudo ifconfig eth0 inet 192.168.19.100  netmask 255.255.255.0  broadcast 192.168.19.1
# 启动 tita_bringup
echo ">>> 启动 tita_bringup ..."
source /opt/d1_ros2/namespace.sh
source install/setup.bash

echo "机器人命名空间设置为: $ROBOT_NS"
ros2 launch tita_bringup sdk_launch.py &
# 记录后台进程 PID，方便后续管理
LAUNCH_PID=$!
# 等待主进程退出
wait $LAUNCH_PID