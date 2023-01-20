/**
 * @file lane_keeping_system.h
 * @author Seungho Hyeong (slkumquat@gmail.com)
 * @brief Lane Keeping System Class header file
 * @version 1.0
 * @date 2023-01-19
 */
#ifndef LANE_KEEPING_SYSTEM_H_
#define LANE_KEEPING_SYSTEM_H_

#include <ros/ros.h>
#include <sensor_msgs/Image.h>
#include <xycar_msgs/xycar_motor.h>
#include <yaml-cpp/yaml.h>

#include "lane_keeping_system/hough_transform_lane_detector.h"
#include "lane_keeping_system/moving_average_filter.h"
#include "lane_keeping_system/pid_controller.h"

#include "yolov3_trt_ros/BoundingBox.h"
#include "yolov3_trt_ros/BoundingBoxes.h"

namespace xycar {

class LaneKeepingSystem {
public:
  // Construct a new Lane Keeping System object
  LaneKeepingSystem();
  // Destroy the Lane Keeping System object
  virtual ~LaneKeepingSystem();
  // Running Lane Keeping System Algorithm
  void run();
  // Set parameters from config file
  void setParams(const YAML::Node &config);

private:
  // Setting steering angle and speed
  void speed_control();

  // Puslish Xycar Moter Message
  void drive(float steering_angle);

  // Subcribe Image Topic
  void imageCallback(const sensor_msgs::Image &msg);

  // Subscribe Yolo Topic
  void yoloCallback(const yolov3_trt_ros::BoundingBoxes& msg);

  // Reset Traffic sign
  void TrafficSignReset();

private:
  // Xycar Steering Angle Limit
  static const int kXycarSteeringAngleLimit = 50;
  // PID Class for Control
  PID *pid_ptr_;
  // Moving Average Filter Class for Noise filtering
  MovingAverageFilter *ma_filter_ptr_;
  // Hough Transform Lane Detector Class for Lane Detection
  HoughTransformLaneDetector *hough_transform_lane_detector_ptr_;

  // ROS Variables
  ros::NodeHandle nh_;
  ros::Publisher pub_;
  ros::Subscriber sub_image_;
  ros::Subscriber sub_yolo_;
  std::string pub_topic_name_;
  std::string sub_image_topic_name_;
  std::string sub_yolo_topic_name_;
  int queue_size_;
  xycar_msgs::xycar_motor msg_;

  // OpenCV Image processing Variables
  cv::Mat frame_;

  // Xycar Device variables
  float steering_angle_ = 0;
  float xycar_speed_;
  float xycar_max_speed_;
  float xycar_min_speed_;
  float xycar_speed_control_threshold_;
  float acceleration_step_;
  float deceleration_step_;
  float choose_lane_threshold_;
  float error_multiplication_;
  int lane_distance_;
  
  // Previous Lane position
  int previous_lpos_ = 0;
  int previous_rpos_ = 640;

  // For Stop line
  double time_;
  double stop_time_;
  bool is_stop_ = false;
  bool is_stopping_ = false;
  bool is_stop_end_ = true;
  float keep_stop_time_;
  float ignore_stop_time_;

  // For Traffic sign
  bool is_traffic_light_ = false;
  bool is_red_ = false;
  bool is_green_ = false;
  bool is_yello_ = false;

  // For Traffic sign
  bool is_left_sign_ = false;
  bool is_right_sign_ = false;
  bool is_stop_sign_ = false;
  bool is_crosswalk_sign_ = false;

  // For Traffic sign Reset
  bool is_traffic_sign_reset_ = false;
  double traffic_sign_reset_time_;
  float reset_time_;

  // Debug Flag
  bool debug_;
};
}  // namespace xycar

#endif  // LANE_KEEPING_SYSTEM_H_
