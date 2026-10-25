cmake_minimum_required(VERSION 3.22)

project(MirrorAppTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui)

# Aspect Ratio Unit Test
add_executable(test_aspect_ratio
    test_aspect_ratio.cpp
)
target_link_libraries(test_aspect_ratio PRIVATE Qt6::Core Qt6::Gui)
add_test(NAME AspectRatioTest COMMAND test_aspect_ratio)

# Coordinate Mapping Unit Test
add_executable(test_coordinate_mapping
    test_coordinate_mapping.cpp
    ../src/input/InputController.cpp
)
target_include_directories(test_coordinate_mapping PRIVATE ../src)
target_link_libraries(test_coordinate_mapping PRIVATE Qt6::Core Qt6::Gui)
add_test(NAME CoordinateMappingTest COMMAND test_coordinate_mapping)
