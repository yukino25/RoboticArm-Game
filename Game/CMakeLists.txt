cmake_minimum_required(VERSION 3.16)
project(RoboticArmGame)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(SDL)

add_executable(game main.cpp)
target_link_libraries(game PRIVATE SDL3::SDL3)
