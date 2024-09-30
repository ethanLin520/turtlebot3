// Copyright 2019 ROBOTIS CO., LTD.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Authors: Taehun Lim (Darby), Ryan Shim

#ifndef WALL_FOLLOWER_HPP_
#define WALL_FOLLOWER_HPP_

#include <geometry_msgs/msg/twist.hpp>
#include <nav_msgs/msg/odometry.hpp>
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/laser_scan.hpp>
#include <tf2/LinearMath/Matrix3x3.h>
#include <tf2/LinearMath/Quaternion.h>
#include "cmath"


#define DEG2RAD (M_PI / 180.0)
#define RAD2DEG (180.0 / M_PI)

#define FRONT		0
#define FRONT_LEFT	1
#define LEFT_FRONT	2
#define LEFT		3
#define LEFT_BACK	4
#define BACK_LEFT	5
#define BACK		6
#define BACK_RIGHT	7
#define RIGHT_BACK	8
#define RIGHT		9
#define RIGHT_FRONT	10
#define FRONT_RIGHT	11

#define LINEAR_VELOCITY  0.3
#define ANGULAR_VELOCITY 1.5

#define BASE_FACTOR 0.8

class WallFollower : public rclcpp::Node
{
public:
	WallFollower();
	~WallFollower();

private:
	// ROS topic publishers
	rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;

	// ROS topic subscribers
	rclcpp::Subscription<sensor_msgs::msg::LaserScan>::SharedPtr scan_sub_;
	rclcpp::Subscription<nav_msgs::msg::Odometry>::SharedPtr odom_sub_;

	// Variables
	double robot_pose_;
	double start_x, start_y;
	bool near_start; 
	double scan_data_[12];
	bool new_scan_data_ = false;
	int since_new_scan = 0;

	// ROS timer
	rclcpp::TimerBase::SharedPtr update_timer_;

	// Function prototypes
	void update_callback();
	void update_cmd_vel(double linear, double angular, double factor = 1);
	void scan_callback(const sensor_msgs::msg::LaserScan::SharedPtr msg);
	void odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg);
};
#endif  // TURTLEBOT3_GAZEBO__TURTLEBOT3_DRIVE_HPP_
