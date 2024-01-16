# CMakeLists.txt for gr

cmake_minimum_required(VERSION 3.10)
project(gr)

# Add source files
set(GR_SOURCES
    GlobalRouter.cpp
    GRNet.cpp
    MazeRoute.cpp
    PatternRoute.cpp
)

# Add library target
add_library(gr ${GR_SOURCES})
target_include_directories(gr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
