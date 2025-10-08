# CMakeList.txt : CMake project for OpenCFD, include source and define
# project specific logic here.
#

# OpenCFD Executable Target
# 2D Lattice Boltzmann CFD simulation with raylib visualization

# Create the main executable
add_executable(OpenCFD 
    "OpenCFD.cpp" 
    "OpenCFD.h"
)

# Set C++20 standard requirement
target_compile_features(OpenCFD PRIVATE cxx_std_20)

# Configure raylib dependency
set(RAYLIB_ROOT "C:/vcpkg/packages/raylib_x64-windows")

if(EXISTS "${RAYLIB_ROOT}")
    message(STATUS "Found raylib at: ${RAYLIB_ROOT}")
    
    # Add raylib include directory
    target_include_directories(OpenCFD PRIVATE "${RAYLIB_ROOT}/include")
    
    # Link raylib library
    if(EXISTS "${RAYLIB_ROOT}/lib/raylib.lib")
        target_link_libraries(OpenCFD PRIVATE "${RAYLIB_ROOT}/lib/raylib.lib")
    else()
        message(FATAL_ERROR "raylib.lib not found at ${RAYLIB_ROOT}/lib")
    endif()
    
    # Link required Windows system libraries for raylib
    if(WIN32)
        target_link_libraries(OpenCFD PRIVATE 
            winmm      # Windows Multimedia API
            gdi32      # Graphics Device Interface
            opengl32   # OpenGL
        )
    endif()
    
else()
    message(FATAL_ERROR "raylib not found at ${RAYLIB_ROOT}")
    message(STATUS "Please install raylib via vcpkg: vcpkg install raylib:x64-windows")
endif()

# Set executable properties
set_target_properties(OpenCFD PROPERTIES
    OUTPUT_NAME "OpenCFD"
    DEBUG_POSTFIX "_d"
)

# Enable warnings
if(MSVC)
    target_compile_options(OpenCFD PRIVATE /W4)
else()
    target_compile_options(OpenCFD PRIVATE -Wall -Wextra -Wpedantic)
endif()

# TODO: Add tests and install targets if needed.
