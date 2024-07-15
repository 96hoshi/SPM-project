cmake_minimum_required(VERSION 3.10)
project(SPM_project)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the build type
#set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE Debug)

# Define an option to enable or disable benchmarking
option(ENABLE_BENCHMARK "Enable times benchmarking" OFF)

# Set the flags for the different build types
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O3 -pg -Wall -Wextra -pthread")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Add the preprocessor definition for the benchmarking
if(ENABLE_BENCHMARK)
    add_definitions(-DBENCHMARK)
endif()

# Check if FF_ROOT is defined in the environment, otherwise set it to a default path
if(NOT DEFINED ENV{FF_ROOT})
    set(FF_ROOT "$ENV{HOME}/include/fastflow")
else()
    set(FF_ROOT "$ENV{FF_ROOT}")
endif()

# Source files for sequential target
set(SEQ_SOURCES
    src/wf-sequential.cpp
)

# Source files for FastFlow target
set(FFP_SOURCES
    src/wf-parallel.cpp
)

# Source files for FastFlow target
set(FFF_SOURCES
    src/wf-farm.cpp
)

# Source files for MPI target
set(MPI_SOURCES
    src/mpi-parallel.cpp
)

# Add executable for sequential target 
add_executable(wf_sequential ${SEQ_SOURCES})
target_include_directories(wf_sequential PRIVATE include)

# Add executable for FastFlow target
add_executable(wf_parallel ${FFP_SOURCES})
target_include_directories(wf_parallel PRIVATE ${FF_ROOT} include)

# Add executable for FastFlow target
add_executable(wf_farm ${FFF_SOURCES})
target_include_directories(wf_farm PRIVATE ${FF_ROOT} include)

# Add executable for MPI target
add_executable(mpi_parallel ${MPI_SOURCES})
target_include_directories(mpi_parallel PRIVATE include)

# Link MPI
find_package(MPI REQUIRED)
target_include_directories(mpi_parallel PRIVATE ${MPI_CXX_INCLUDE_PATH})
target_link_libraries(mpi_parallel ${MPI_CXX_LIBRARIES})
