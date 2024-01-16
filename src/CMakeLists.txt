# CMakeLists.txt for src

cmake_minimum_required(VERSION 3.10)
project(main)

# Add source files
set(MAIN_SOURCES
    main.cpp
)

add_subdirectory(basic)  # Make sure this line is before add_executable(main ...)
add_subdirectory(gr)  # Add this line to include gr directory

add_executable(main ${MAIN_SOURCES})
target_include_directories(main PRIVATE basic gr)  # Add gr to the include directories
target_link_libraries(main PRIVATE basic gr stdc++fs)  # Add gr to the linked libraries
