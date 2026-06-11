cmake_minimum_required(VERSION 3.8)
project(drone_fleet)

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

# Include paths for your C++ headers
include_directories(
  include
  ../../../part1/include
)

# 1. SETUP THE DRONE NODE TARGET
add_executable(drone_node
  src/drone_node.cpp
  ../../../part1/src/vehicle.cpp
  ../../../part1/src/drone.cpp
  ../../../part1/src/mission_drone.cpp
)
ament_target_dependencies(drone_node rclcpp std_msgs)

# 2. SETUP THE FLEET MANAGER TARGET (Add these two lines!)
add_executable(fleet_manager_node src/fleet_manager.cpp)
ament_target_dependencies(fleet_manager_node rclcpp std_msgs)

add_executable(health_monitor_node src/health_monitor.cpp)
ament_target_dependencies(health_monitor_node rclcpp std_msgs)

# 3. INSTALL CONFIGURATIONS (Now safely installs both executables)
install(TARGETS
    drone_node
    fleet_manager_node
    health_monitor_node
    DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()