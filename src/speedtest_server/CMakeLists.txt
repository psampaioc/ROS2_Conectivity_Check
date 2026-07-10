cmake_minimum_required(VERSION 3.8)

add_executable(speedtest_server speedtest_server.cpp)
ament_target_dependencies(speedtest_server
  rclcpp
  std_srvs
  std_msgs
  builtin_interfaces
)
rosidl_target_interfaces(speedtest_server ${PROJECT_NAME} "rosidl_typesupport_cpp")

install(TARGETS speedtest_server
  DESTINATION lib/${PROJECT_NAME}
)