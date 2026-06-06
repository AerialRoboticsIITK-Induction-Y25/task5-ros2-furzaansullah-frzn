cmake_minimum_required(VERSION 3.10)
project(DroneFleetManager LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall -Wextra)

# 1. Tell CMake to look inside the "include" folder for header files
include_directories(include)

# 2. Add the "src/" prefix to all your source files
set(SOURCES
    src/vehicle.cpp
    src/drone.cpp
    src/mission_drone.cpp
    src/autonomous_drone.cpp
    src/main.cpp
)

add_executable(drone_fleet ${SOURCES})