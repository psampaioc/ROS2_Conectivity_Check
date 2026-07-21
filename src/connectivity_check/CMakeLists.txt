cmake_minimum_required(VERSION 3.8)
project(connectivity_check LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNL REQUIRED libnl-3.0 libnl-genl-3.0)

include_directories(include ${LIBNL_INCLUDE_DIRS})

add_executable(rss_node src/rss_node.cpp)

ament_target_dependencies(rss_node rclcpp std_msgs)
target_link_libraries(rss_node ${LIBNL_LIBRARIES})

install(TARGETS rss_node
  RUNTIME DESTINATION lib/${PROJECT_NAME}
)

ament_package()