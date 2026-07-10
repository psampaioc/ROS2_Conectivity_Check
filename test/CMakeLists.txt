cmake_minimum_required(VERSION 3.8)

if(BUILD_TESTING)
  find_package(GTest REQUIRED)

  add_executable(test_messages test_messages.cpp)
  ament_target_dependencies(test_messages
    rclcpp
    std_msgs
    builtin_interfaces
  )
  # Link against generated message typesupport for same-package messages
  rosidl_target_interfaces(test_messages ${PROJECT_NAME} "rosidl_typesupport_cpp")
  target_link_libraries(test_messages GTest::GTest GTest::Main)

  add_test(NAME test_messages COMMAND test_messages)
endif()