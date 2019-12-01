# OdomCal
## calibrate the parameters of odometer

## 1. Prerequisites
### 1.1 **Ubuntu** and **ROS**
Ubuntu 64-bit 14.04 or 16.04.
ROS Kinetic or Indigo. [ROS Installation](http://wiki.ros.org/ROS/Installation)

### 1.2. **Others**
OpenCV 3.3 , Eigen3

## 2. Build
Clone the repository and catkin_make:
```
    cd ~/catkin_ws/src
    git clone xxx
    cd ../
    catkin_make
    source ~/catkin_ws/devel/setup.bash
```
(if you fail in this step, try to find another computer with clean system or reinstall Ubuntu and ROS)


## 3. Attention

### 3.1 record your testBag
record the odom datas and observation datas in txt file.

### 3.2 Configuration file
Write your parameters customerly in launch file according to your configuration.

### 3.3 Remind

The porgram need 2 different path files and one observation file at least.
One path corresponds to one txt file. For example odom_1.txt.
"observations.txt" is measurements of paths in world coordinate, one path corresponds to one line in the txt file .
The pattern is start location (3 elements x,y,theta), end location (3 elements x,y,theta).


## 4. Calibration

```
    roslaunch odomcal odomcal.launch
```

## 5. Authors

1. Hai Dan.

2. Zhang Yu

3. Li Jingyu

## 6. References

Gianluca Antonelli, Stefano Chiaverini. Linear estimation of the physical odometric parameters for differential-drive mobile robots.Auton Robot (2007) 23: 59–68

