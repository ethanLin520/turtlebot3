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
//
// Modified by Claude Sammut for COMP3431
// Use this code as the basis for a wall follower

#include "wall_follower/wall_follower.hpp"

#include <memory>


using namespace std::chrono_literals;

WallFollower::WallFollower()
: Node("wall_follower_node")
{
	/************************************************************
	** Initialise variables
	************************************************************/
	for (int i = 0; i < 12; i++)
		scan_data_[i] = 0.0;

	robot_pose_ = 0.0;
	near_start = false;

	/************************************************************
	** Initialise ROS publishers and subscribers
	************************************************************/
	auto qos = rclcpp::QoS(rclcpp::KeepLast(10));

	// Initialise publishers
	cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", qos);

	// Initialise subscribers
	scan_sub_ = this->create_subscription<sensor_msgs::msg::LaserScan>(
		"scan", \
		rclcpp::SensorDataQoS(), \
		std::bind(
			&WallFollower::scan_callback, \
			this, \
			std::placeholders::_1));
	odom_sub_ = this->create_subscription<nav_msgs::msg::Odometry>(
		"odom", qos, std::bind(&WallFollower::odom_callback, this, std::placeholders::_1));

	/************************************************************
	** Initialise ROS timers
	************************************************************/
	update_timer_ = this->create_wall_timer(100ms, std::bind(&WallFollower::update_callback, this));

	RCLCPP_INFO(this->get_logger(), "Wall follower node has been initialised");
}

WallFollower::~WallFollower()
{
	RCLCPP_INFO(this->get_logger(), "Wall follower node has been terminated");
}

/********************************************************************************
** Callback functions for ROS subscribers
********************************************************************************/

#define START_RANGE	0.2

void WallFollower::odom_callback(const nav_msgs::msg::Odometry::SharedPtr msg)
{
	static bool first = true;
	static bool start_moving = true;

	tf2::Quaternion q(
		msg->pose.pose.orientation.x,
		msg->pose.pose.orientation.y,
		msg->pose.pose.orientation.z,
		msg->pose.pose.orientation.w);
	tf2::Matrix3x3 m(q);
	double roll, pitch, yaw;
	m.getRPY(roll, pitch, yaw);

	robot_pose_ = yaw;

	double current_x =  msg->pose.pose.position.x;
	double current_y =  msg->pose.pose.position.y;
	if (first)
	{
		start_x = current_x;
		start_y = current_y;
		first = false;
	}
	else if (start_moving)
	{
		if (fabs(current_x - start_x) > START_RANGE || fabs(current_y - start_y) > START_RANGE)
			start_moving = false;
	}
	else if (fabs(current_x - start_x) < START_RANGE && fabs(current_y - start_y) < START_RANGE)
	{
		fprintf(stderr, "Near start!!\n");
		near_start = true;
		first = true;
		start_moving = true;
	}
	RCLCPP_INFO(this->get_logger(), "Position (x: %f, y: %f), Orientation (yaw: %f)", 
            msg->pose.pose.position.x, 
            msg->pose.pose.position.y, 
            robot_pose_);
}

#define BEAM_WIDTH 10

void WallFollower::scan_callback(const sensor_msgs::msg::LaserScan::SharedPtr msg)
{
	if (!initial_scan) initial_scan = true;
	new_scan_data = true;
	uint16_t scan_angle[12] = {0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330};

	double closest = msg->range_max;
	for (int angle = 360-BEAM_WIDTH; angle < 360; angle++)
		if (msg->ranges.at(angle) < closest)
			closest = msg->ranges.at(angle);
	for (int angle = 0; angle < BEAM_WIDTH; angle++)
		if (msg->ranges.at(angle) < closest)
			closest = msg->ranges.at(angle);
	scan_data_[0] = closest;

	for (int i = 1; i < 12; i++)
	{
		closest = msg->range_max;
		for (int angle = scan_angle[i]-BEAM_WIDTH; angle < scan_angle[i]+BEAM_WIDTH; angle++)
			if (msg->ranges.at(angle) < closest)
				closest = msg->ranges.at(angle);
		scan_data_[i] = closest;
	}

	RCLCPP_INFO(this->get_logger(), "Closest distance in front: %f", scan_data_[0]);
}

void WallFollower::update_cmd_vel(double linear, double angular, double factor)
{
	geometry_msgs::msg::Twist cmd_vel;
	cmd_vel.linear.x = linear * factor;
	cmd_vel.angular.z = angular * factor;

	cmd_vel_pub_->publish(cmd_vel);
}

/********************************************************************************
** Update functions
********************************************************************************/

bool pl_near;


void WallFollower::update_callback()
{
	if (!initial_scan) return;

	if (new_scan_data) {
		new_scan_data = false;
		since_new_scan = 0;
	} else {
		since_new_scan++;
	}

	update_velocity();
}

void WallFollower::update_velocity() {
	double factor = pow(BASE_FACTOR, since_new_scan);	// e.g. 0.8 ^ n

	if (near_start) {
        RCLCPP_INFO(this->get_logger(), "Near start detected, stopping the robot.");
        update_cmd_vel(0.0, 0.0, factor);
    }
    else if (scan_data_[LEFT_FRONT] > 0.9) {
        RCLCPP_INFO(this->get_logger(), "Left front clear, turning left. Linear: 0.2, Angular: 1.5");
        update_cmd_vel(0.2, 1.5, factor);
    }
    else if (scan_data_[FRONT] < 0.7) {
        RCLCPP_INFO(this->get_logger(), "Obstacle ahead, turning right. Linear: 0.0, Angular: -1.5");
        update_cmd_vel(0.0, -1.5, factor);
    }
    else if (scan_data_[FRONT_LEFT] < 0.6) {
        RCLCPP_INFO(this->get_logger(), "Front left obstacle, turning right. Linear: 0.3, Angular: -1.5");
        update_cmd_vel(0.3, -1.5, factor);
    }
    else if (scan_data_[FRONT_RIGHT] < 0.6) {
        RCLCPP_INFO(this->get_logger(), "Front right obstacle, turning left. Linear: 0.3, Angular: 1.5");
        update_cmd_vel(0.3, 1.5, factor);
    }
    else if (scan_data_[LEFT_FRONT] > 0.6) {
        RCLCPP_INFO(this->get_logger(), "Left front clear, moving forward with slight left turn. Linear: 0.3, Angular: 1.5");
        update_cmd_vel(0.3, 1.5, factor);
    }
    else {
        RCLCPP_INFO(this->get_logger(), "Path clear, moving forward. Linear: 0.3, Angular: 0.0");
        update_cmd_vel(0.3, 0.0, factor);
    }
}



/*******************************************************************************
** Main
*******************************************************************************/
int main(int argc, char ** argv)
{
	rclcpp::init(argc, argv);
	rclcpp::spin(std::make_shared<WallFollower>());
	rclcpp::shutdown();

	return 0;
}
