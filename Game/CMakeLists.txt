cmake_minimum_required(VERSION 3.16)
project(RoboticArmGame)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.10
)
FetchContent_MakeAvailable(SDL3)

# Game executable
add_executable(game
    main.cpp
    gravity.cpp
    movement.cpp
    level.cpp
    render.cpp
)
target_include_directories(game PRIVATE .)
target_link_libraries(game PRIVATE SDL3::SDL3)

# Unit tests (no SDL dependency)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(tests
    tests/test_physics.cpp
    tests/test_movement.cpp
    gravity.cpp
    movement.cpp
)
target_include_directories(tests PRIVATE .)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
