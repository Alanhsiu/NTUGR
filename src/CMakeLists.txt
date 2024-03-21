# CMakeLists.txt for src

# Specify the minimum version for CMake
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 11)

# Project's name
project(NTUGR)

# Set the default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# Message
message(STATUS "Global Router CUGR 2.0")
message(STATUS "Current source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "Project source directory: ${PROJECT_SOURCE_DIR}")

# Add source files
set(MAIN_SOURCES
    main.cpp
)

# Add subdirectories for modular organization
add_subdirectory(basic)
add_subdirectory(gr)
add_subdirectory(flute)

# Create the executable
add_executable(route ${MAIN_SOURCES})

# Specify include directories
target_include_directories(route PRIVATE basic gr flute)

# Link libraries
target_link_libraries(route PRIVATE basic gr flute)

# Set the C++ standard
set_property(TARGET route PROPERTY CXX_STANDARD 17)

# Include debug symbols in the Debug build
if(CMAKE_BUILD_TYPE MATCHES Debug)
  target_compile_options(route PRIVATE -g)
endif()

if(CMAKE_BUILD_TYPE MATCHES Release)
  target_compile_options(route PRIVATE -O3 -march=native)
endif()

# Enable testing with CMake
enable_testing()

# Define a Valgrind test for the executable
add_test(NAME ValgrindMemoryCheck
         COMMAND valgrind --tool=memcheck --leak-check=full --show-reachable=yes --error-exitcode=1 $<TARGET_FILE:route>)

# Optional: Add custom test properties for setting environment variables or passing arguments
set_tests_properties(ValgrindMemoryCheck
                     PROPERTIES ENVIRONMENT "VAR=VALUE" # Example for setting environment variables
                                PASS_REGULAR_EXPRESSION "no leaks are possible" # Example for expecting certain output
)

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(route PUBLIC OpenMP::OpenMP_CXX)
endif()