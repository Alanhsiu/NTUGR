# CMakeLists.txt for basic

cmake_minimum_required(VERSION 3.10)
project(basic)

# Add source files
set(BASIC_SOURCES
    design.cpp
)

# Add library target
add_library(basic ${BASIC_SOURCES})
target_include_directories(basic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
