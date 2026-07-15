cmake_minimum_required(VERSION 3.8)

find_package(rclcpp_components REQUIRED)

add_executable(connectivity_monitor main.cpp)
ament_target_dependencies(connectivity_monitor
  rclcpp
  std_msgs
  builtin_interfaces
)
rosidl_target_interfaces(connectivity_monitor ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(connectivity_monitor ${PROJECT_NAME}__rosidl_typesupport_cpp)

install(TARGETS connectivity_monitor
  DESTINATION lib/${PROJECT_NAME}
)