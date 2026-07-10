cmake_minimum_required(VERSION 3.8)

find_package(rclcpp_components REQUIRED)

add_executable(connectivity_monitor connectivity_monitor.cpp main.cpp)
ament_target_dependencies(connectivity_monitor
  rclcpp
  std_msgs
  builtin_interfaces
  rclcpp_components
)
rosidl_target_interfaces(connectivity_monitor ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_include_directories(connectivity_monitor PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../include
  ${LIBNL_INCLUDE_DIRS}
  ${MODEM_MANAGER_INCLUDE_DIRS}
)

install(TARGETS connectivity_monitor
  DESTINATION lib/${PROJECT_NAME}
)