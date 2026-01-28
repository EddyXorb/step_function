cmake_minimum_required(VERSION 3.22)

project(step_function)

find_package(Catch2 REQUIRED)

add_executable(${PROJECT_NAME} src/main.cpp)

target_include_directories(${PROJECT_NAME} PUBLIC include)
target_sources(${PROJECT_NAME} PUBLIC include/step_function.hpp)

target_link_libraries(${PROJECT_NAME} Catch2::Catch2WithMain)
set_target_properties(${PROJECT_NAME} PROPERTIES
            CXX_STANDARD 17
            CXX_EXTENSIONS OFF
            )