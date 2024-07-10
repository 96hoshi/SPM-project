cmake_minimum_required(VERSION 3.10)
project(SPM_project)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the build type
#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)

# Set the flags for the different build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O2 -DBENCHMARK -g ")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG ")

# Check if FF_ROOT is defined in the environment, otherwise set it to a default path
if(NOT DEFINED ENV{FF_ROOT})
    set(FF_ROOT "$ENV{HOME}/include/fastflow")
else()
    set(FF_ROOT "$ENV{FF_ROOT}")
endif()

# Source files for FastFlow target
set(FF_SOURCES
    src/ff-parallel.cpp
)

# Source files for MPI target
set(MPI_SOURCES
    src/mpi-parallel.cpp
)

# Source files for sequential target
set(SEQ_SOURCES
    src/sequential.cpp
)

# Add executable for sequential target 
add_executable(sequential ${SEQ_SOURCES})
target_include_directories(sequential PRIVATE include)

# Add executable for FastFlow target
add_executable(ff_parallel ${FF_SOURCES})
target_include_directories(ff_parallel PRIVATE ${FF_ROOT} include)

# Add executable for MPI target
add_executable(mpi_parallel ${MPI_SOURCES})
target_include_directories(mpi_parallel PRIVATE include)

# Link MPI
find_package(MPI REQUIRED)
target_include_directories(mpi_parallel PRIVATE ${MPI_CXX_INCLUDE_PATH})
target_link_libraries(mpi_parallel ${MPI_CXX_LIBRARIES})
