cmake_minimum_required(VERSION 3.8)

find_package(rclcpp_components REQUIRED)
find_package(yaml-cpp REQUIRED)
find_package(class_loader REQUIRED)
find_package(ament_index_cpp REQUIRED)

add_executable(rss_monitor rss_monitor.cpp
  rss_monitor_main.cpp
  wifi_rss_provider.cpp
  cellular_rss_provider.cpp
  generic_rss_provider.cpp
  rss_provider.cpp
)
ament_target_dependencies(rss_monitor
  rclcpp
  std_msgs
  builtin_interfaces
  rclcpp_components
  yaml-cpp
  class_loader
  ament_index_cpp
)
rosidl_target_interfaces(rss_monitor ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_include_directories(rss_monitor PRIVATE
  ${LIBNL_INCLUDE_DIRS}
  ${MODEM_MANAGER_INCLUDE_DIRS}
)
target_link_libraries(rss_monitor
  ${LIBNL_LIBRARIES}
  ${MODEM_MANAGER_LIBRARIES}
  yaml-cpp
)

install(TARGETS rss_monitor
  DESTINATION lib/${PROJECT_NAME}
)