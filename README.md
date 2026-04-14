

​	D1 Langyi Control Demo


## Build Package

```bash
git clone https://github.com/DDTRobot/TITA-SDK-ROS2.git
colcon build
bash start_D1.sh
```

robot@d1:~/TITA-SDK-ROS2$ bash start_D1.sh
[sudo] password for robot:
>>> 启动 tita_bringup ...
Robot Namespace is d13007898
机器人命名空间设置为: d13007898
[INFO] [launch]: All log files can be found below /home/robot/.ros/log/2026-04-14-03-19-57-344925-d1-1263
[INFO] [launch]: Default logging verbosity is set to INFO
[INFO] [user_command_node-1]: process started with pid [1264]
[user_command_node-1] [INFO] [1776136797.424797760] [user_command_node]: User SDK with UDP receiver! Server IP: 192.168.19.101, Robot NS: d13007898
[user_command_node-1] [INFO] [1776136797.424993728] [user_command_node]: Publishing to topic: /d13007898/command/user_command
[user_command_node-1] [INFO] [1776136797.427032640] [user_command_node]: 发布初始状态机模式: transform_up
[user_command_node-1] [INFO] [1776136797.427231072] [user_command_node]: UDP receiver started successfully.