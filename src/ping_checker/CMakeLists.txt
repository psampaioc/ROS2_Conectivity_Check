cmake_minimum_required(VERSION 3.8)

find_package(rclcpp_components REQUIRED)
find_package(yaml-cpp REQUIRED)

add_executable(ping_checker ping_checker.cpp ping_checker_main.cpp)
ament_target_dependencies(ping_checker
  rclcpp
  std_msgs
  builtin_interfaces
  yaml-cpp
  rclcpp_components
)
rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")
target_link_libraries(ping_checker ${cpp_typesupport_target} yaml-cpp)

install(TARGETS ping_checker
  DESTINATION lib/${PROJECT_NAME}
)