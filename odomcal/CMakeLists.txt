cmake_minimum_required(VERSION 2.8.3)
project(odomcal)
add_compile_options(-std=c++11)
find_package(Boost REQUIRED)
## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
set(CMAKE_BUILD_TYPE debug)
include_directories("/usr/include/eigen3/")
find_package(catkin REQUIRED COMPONENTS roscpp geometry_msgs turtlesim std_msgs serial tf nav_msgs visualization_msgs)
find_package(OpenCV REQUIRED)
catkin_package()
include_directories(include ${catkin_INCLUDE_DIRS})
add_executable(odomcal odomcal.cpp)
target_link_libraries(odomcal ${catkin_LIBRARIES} ${OpenCV_LIBS})

