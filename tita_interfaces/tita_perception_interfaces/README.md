<p align="center"><strong>tita_perception_interfaces</strong></p>
<p align="center"><a href="https://github.com/${YOUR_GIT_REPOSITORY}/blob/main/LICENSE"><img alt="License" src="https://img.shields.io/badge/License-Apache%202.0-orange"/></a>
<img alt="language" src="https://img.shields.io/badge/language-c++-red"/>
<img alt="platform" src="https://img.shields.io/badge/platform-linux-l"/>
</p>
<p align="center">
    语言：<a href="./docs/docs_en/README_EN.md"><strong>English</strong></a> / <strong>中文</strong>
</p>



​	机器人感知相关的自定义接口。

## Basic Information

| Installation method | Supported platform[s]    |
| ------------------- | ------------------------ |
| Source              | Jetpack 6.0 , ros-humble |

------

## BoundingBox.msg

|       Type        |   Name   |       Description        |
| :---------------: | :------: | :----------------------: |
| `std_msgs/Header` | `header` |     ROS2 标准消息头      |
|     `float64`     | `x_min`  | Bounding box 中 x 最小值 |
|     `float64`     | `y_min`  | Bounding box 中 y 最小值 |
|     `float64`     | `x_max`  | Bounding box 中 x 最大值 |
|     `float64`     | `y_max`  | Bounding box 中 y 最大值 |
|     `string`      | `label`  |         识别 ID          |

## BoundingBox.msg

|       Type        |   Name   |          Description           |
| :---------------: | :------: | :----------------------------: |
| `std_msgs/Header` | `header` |        ROS2 标准消息头         |
|  `BoundingBox[]`  | `boxes`  | 由 Bounding box 消息组成的数组 |

## ObjectDetection.srv

|        Type         |   Name   |   Description    |
| :-----------------: | :------: | :--------------: |
| `sensor_msgs/Image` | `image`  | 请求发送一张图片 |
| `BoundingBoxArray`  | `result` |  返回识别的结果  |

## 

## Build Package

```bash
# if have extra dependencies
# apt install <libdepend-dev>
colcon build --packages-select tita_perception_interfaces
```
