# OpenCFD - 2D Lattice Boltzmann CFD Simulation
# Root CMake configuration file

cmake_minimum_required(VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported
if (POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project("OpenCFD" 
    VERSION 1.0.0
    DESCRIPTION "Real-time 2D CFD simulation using Lattice Boltzmann Method"
    LANGUAGES CXX
)

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Add the main executable project
add_subdirectory("OpenCFD")
