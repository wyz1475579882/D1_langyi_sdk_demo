<p align="center"><strong>tita_locomotion_interfaces</strong></p>
<p align="center"><a href="https://github.com/${YOUR_GIT_REPOSITORY}/blob/main/LICENSE"><img alt="License" src="https://img.shields.io/badge/License-Apache%202.0-orange"/></a>
<img alt="language" src="https://img.shields.io/badge/language-c++-red"/>
<img alt="platform" src="https://img.shields.io/badge/platform-linux-l"/>
</p>
<p align="center">
    语言：<a href="./docs/docs_en/README_EN.md"><strong>English</strong></a> / <strong>中文</strong>
</p>


​	机器人运动控制相关的自定义消息接口。

## Basic Information

| Installation method | Supported platform[s]    |
| ------------------- | ------------------------ |
| Source              | Jetpack 6.0 , ros-humble |

------

## LocomotionCmd.msg

|         Type          |    Name    |       Description        |
| :-------------------: | :--------: | :----------------------: |
|   `std_msgs/Header`   |  `header`  |     ROS2 标准消息头      |
|       `string`        | `fsm_mode` |        机器人状态        |
| `geometry_msgs/Pose`  |   `pose`   | 机器人躯干的姿态控制信息 |
| `geometry_msgs/Twist` |  `twist`   | 机器人底盘的速度控制信息 |

## Build Package

```bash
# if have extra dependencies
# apt install <libdepend-dev>
colcon build --packages-select tita_locomotion_interfaces
```
