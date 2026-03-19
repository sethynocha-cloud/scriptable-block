cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ScriptableBlock VERSION 1.1.0)

# Collect all source files recursively
file(GLOB_RECURSE SOURCES src/*.cpp)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src/)

if(NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR
        "The GEODE_SDK environment variable is not set. "
        "Please set it to the path of your Geode SDK installation."
    )
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)
setup_geode_mod(${PROJECT_NAME})
