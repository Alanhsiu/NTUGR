# CMakeLists.txt for gr

cmake_minimum_required(VERSION 3.10)
project(gr)

# Add source files
set(GR_SOURCES
    GlobalRouter.cpp
    GridGraph.cpp
    GRNet.cpp
    GRTree.cpp
    MazeRoute.cpp
    PatternRoute.cpp
)

# Add gpulibrary
# add_library(grgpu XXX.cu)

# Add library target
add_library(gr ${GR_SOURCES})
target_include_directories(gr PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_link_libraries(gr PUBLIC grgpu)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(gr PUBLIC OpenMP::OpenMP_CXX)
endif()