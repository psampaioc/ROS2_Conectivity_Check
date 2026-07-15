cmake_minimum_required(VERSION 3.8)

find_package(rclcpp_components REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(ament_index_cpp REQUIRED)

# System deps for netlink
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNL REQUIRED libnl-3.0 libnl-genl-3.0)

add_executable(rss_monitor rss_monitor.cpp)
ament_target_dependencies(rss_monitor
  rclcpp
  std_msgs
  builtin_interfaces
  rclcpp_components
  yaml-cpp
  ament_index_cpp
)
rosidl_target_interfaces(rss_monitor ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_include_directories(rss_monitor PRIVATE ${LIBNL_INCLUDE_DIRS})
target_link_libraries(rss_monitor ${LIBNL_LIBRARIES} yaml-cpp)

install(TARGETS rss_monitor
  DESTINATION lib/${PROJECT_NAME}
)