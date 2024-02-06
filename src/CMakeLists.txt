# CMakeLists.txt for src

# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.10)

# Message
message(STATUS "Global Router CUGR 2.0")
message(STATUS ${CMAKE_CURRENT_SOURCE_DIR})
message(STATUS ${PROJECT_SOURCE_DIR})

# Project's name
project(NTUGR)

# Add source files
set(MAIN_SOURCES
    main.cpp
)

add_subdirectory(basic)  # Make sure this line is before add_executable(main ...)
add_subdirectory(gr)  # Add this line to include gr directory
add_subdirectory(flute)  # Add this line to include flute directory


add_executable(route ${MAIN_SOURCES})
target_include_directories(route PRIVATE basic gr flute)  # Add flute to the include directories
target_link_libraries(route PRIVATE basic gr stdc++fs flute)  # Add flute to the linked libraries
