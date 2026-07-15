cmake_minimum_required(VERSION 3.8)

add_executable(speedtest_server speedtest_server.cpp)
ament_target_dependencies(speedtest_server
  rclcpp
  std_srvs
  std_msgs
  builtin_interfaces
)
rosidl_get_typesupport_target(cpp_typesupport_target "${PROJECT_NAME}" "rosidl_typesupport_cpp")
target_link_libraries(speedtest_server ${cpp_typesupport_target})

install(TARGETS speedtest_server
  DESTINATION lib/${PROJECT_NAME}
)