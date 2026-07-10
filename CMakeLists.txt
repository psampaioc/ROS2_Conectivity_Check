cmake_minimum_required(VERSION 3.8)
project(conectivity_check LANGUAGES CXX C)

# ============================================
# C++ Standard
# ============================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# C Standard (needed for rosidl C code generation)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ============================================
# Find Packages
# ============================================
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(builtin_interfaces REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# System dependencies for RSS providers
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNL REQUIRED libnl-3.0 libnl-genl-3.0)
pkg_check_modules(MODEM_MANAGER REQUIRED mm-glib)
find_package(yaml-cpp REQUIRED)

# ============================================
# ROS 2 Interfaces (Messages + Services)
# ============================================
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/PingResult.msg"
  "msg/PingSummary.msg"
  "msg/RssMeasurement.msg"
  "msg/RssSummary.msg"
  "msg/SpeedtestResult.msg"
  "msg/ConnectivitySummary.msg"
  "srv/TriggerSpeedtest.srv"
  DEPENDENCIES builtin_interfaces std_msgs
)

# ============================================
# Include Directories
# ============================================
include_directories(include)

# Make system library include directories available to all targets
include_directories(${LIBNL_INCLUDE_DIRS} ${MODEM_MANAGER_INCLUDE_DIRS})

# ============================================
# Subdirectories (Nodes)
# ============================================
add_subdirectory(src/connectivity_monitor)
add_subdirectory(src/ping_checker)
add_subdirectory(src/rss_monitor)
add_subdirectory(src/speedtest_server)

# Ensure message generation happens before building C++ targets
# rosidl_target_interfaces in each subdirectory handles the includes automatically
if(TARGET ${PROJECT_NAME}__rosidl_typesupport_cpp)
  add_dependencies(connectivity_monitor ${PROJECT_NAME}__rosidl_typesupport_cpp)
  add_dependencies(ping_checker ${PROJECT_NAME}__rosidl_typesupport_cpp)
  add_dependencies(rss_monitor ${PROJECT_NAME}__rosidl_typesupport_cpp)
  add_dependencies(speedtest_server ${PROJECT_NAME}__rosidl_typesupport_cpp)
endif()

# ============================================
# Install
# ============================================
install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

# Tests
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  find_package(ament_lint_common REQUIRED)
  ament_lint_auto_find_test_dependencies()
  find_package(GTest REQUIRED)
  add_subdirectory(test)
  # Ensure test target depends on message generation
  if(TARGET ${PROJECT_NAME}__rosidl_typesupport_cpp AND TARGET test_messages)
    add_dependencies(test_messages ${PROJECT_NAME}__rosidl_typesupport_cpp)
  endif()
endif()

ament_package()