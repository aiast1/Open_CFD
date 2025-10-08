# OpenCFD - 2D Lattice Boltzmann CFD Simulation

A real-time 2D Computational Fluid Dynamics (CFD) simulation using the Lattice Boltzmann Method (LBM) with raylib visualization. This project demonstrates **Karman vortex streets** - the beautiful alternating vortices that form behind a circular obstacle in fluid flow.

![CFD Visualization](https://img.shields.io/badge/CFD-Lattice_Boltzmann-blue) ![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=c%2B%2B) ![Raylib](https://img.shields.io/badge/Graphics-raylib-red) ![CMake](https://img.shields.io/badge/Build-CMake-064F8C?logo=cmake)

## Features

- **Real-time CFD simulation** using D2Q9 Lattice Boltzmann Method
- **Live visualization** with raylib graphics library
- **Karman vortex street** formation around circular obstacles
- **Color-coded velocity field** visualization (blue=low speed, red=high speed)
- **Interactive physics** with proper fluid dynamics:
  - BGK collision operator
  - Streaming step for particle distribution
  - Bounce-back boundary conditions
  - Inlet/outlet flow boundaries
  - Equilibrium distribution functions

## Preview

The simulation shows fluid flowing from left to right around a gray circular obstacle, creating the characteristic alternating vortex shedding pattern known as Karman vortex streets. The velocity field is visualized with a smooth color gradient.

## Requirements

### System Requirements
- **Windows 10/11** (x64)
- **Visual Studio 2019/2022** with C++ development tools
- **CMake 3.8+** (tested with 3.31.6)

### Dependencies
- **raylib** (graphics library) - installed via vcpkg
- **C++20** standard library

## Installation

### 1. Install vcpkg and raylib

```bash
# Clone vcpkg (if not already installed)
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# Install raylib for x64-windows
.\vcpkg install raylib:x64-windows
```

### 2. Clone and Build

```bash
# Clone the repository
git clone <repository-url>
cd OpenCFD

# Configure with CMake (x64 build)
cmake -B build -S . -G "Visual Studio 17 2022" -A x64

# Build the project
cmake --build build --config Release
```

### 3. Run the Simulation

```bash
# Run the executable
.\build\OpenCFD\Release\OpenCFD.exe
```

## Project Structure

```
OpenCFD/
??? OpenCFD/
?   ??? OpenCFD.cpp         # Main simulation and rendering code
?   ??? OpenCFD.h           # Headers and raylib includes
?   ??? CMakeLists.txt      # Project-specific CMake config
??? CMakeLists.txt          # Root CMake configuration
??? CMakePresets.json       # CMake presets for development
??? .gitignore             # Git ignore rules
??? README.md              # This file
```

## Technical Details

### Lattice Boltzmann Method (LBM)
- **Model**: D2Q9 (2D with 9 discrete velocities)
- **Collision Model**: BGK (Bhatnagar-Gross-Krook)
- **Grid Size**: 400×200 cells
- **Boundary Conditions**:
  - Bounce-back for solid obstacles
  - Velocity inlet (left boundary)
  - Pressure outlet (right boundary)
  - Periodic top/bottom boundaries

### Physics Parameters
- **Reynolds Number**: ~100 (adjustable)
- **Inlet Velocity**: 0.08 (lattice units)
- **Relaxation Time**: Automatically calculated from viscosity
- **Obstacle**: Circular cylinder at 1/4 domain width

### Rendering
- **Graphics**: raylib 5.5
- **Resolution**: 800×400 pixels (2× scaled)
- **Frame Rate**: 60 FPS target
- **Visualization**: Real-time velocity magnitude coloring

## Customization

### Modify Simulation Parameters

Edit `OpenCFD.cpp` to change:

```cpp
// Domain size
static const int NX = 400;  // Grid width
static const int NY = 200;  // Grid height

// Simulation parameters
const float u_in = 0.08f;   // Inlet velocity
const float Re = 100.0f;    // Reynolds number
const float radius = NY / 9.0f;  // Cylinder radius
```

### Obstacle Shape
Currently uses a circular obstacle. Modify the obstacle mask generation in `main()` to create different shapes:

```cpp
// Current: circular obstacle
if (dx*dx + dy*dy <= radius*radius) obstacle[idx(x,y)] = 1;

// Example: rectangular obstacle
// if (abs(dx) <= width/2 && abs(dy) <= height/2) obstacle[idx(x,y)] = 1;
```

## Development

### Building for Debug

```bash
cmake --build build --config Debug
```

### Code Structure
- **LBM Core**: Collision and streaming steps in main loop
- **Boundary Conditions**: Applied after streaming
- **Visualization**: Texture updates with velocity field coloring
- **Performance**: Multiple LBM steps per frame for faster dynamics

## Physics Background

The **Lattice Boltzmann Method** is a computational approach to fluid dynamics that:
1. Discretizes space into a regular lattice
2. Uses particle distribution functions at each lattice point
3. Evolves the system through collision and streaming steps
4. Recovers macroscopic fluid behavior (Navier-Stokes equations)

**Karman Vortex Streets** occur when fluid flows past a bluff body (like a cylinder) at moderate Reynolds numbers (Re ~40-200), creating periodic vortex shedding that forms the characteristic alternating pattern.

## Performance Notes

- **CPU Implementation**: Runs entirely on CPU (single-threaded)
- **Real-time**: 4 LBM steps per frame at 60 FPS
- **Memory Usage**: ~13 MB for 400×200 grid
- **Future Enhancement**: GPU acceleration possible with compute shaders

## Troubleshooting

### Build Issues
- Ensure raylib is installed in `C:\vcpkg\packages\raylib_x64-windows`
- Use x64 build configuration (not x86)
- Update CMake to 3.8+ if encountering version warnings

### Runtime Issues
- Verify OpenGL drivers are up to date for raylib
- Check Windows Defender isn't blocking the executable

## License

This project is open source. Feel free to use, modify, and distribute.

## Contributing

Contributions welcome! Areas for improvement:
- GPU acceleration with compute shaders
- Additional obstacle shapes
- 3D visualization
- Performance optimizations
- Different LBM models (MRT, etc.)

## References

- Lattice Boltzmann Method: Krüger et al., "The Lattice Boltzmann Method"
- raylib Graphics Library: https://www.raylib.com/
- Karman Vortex Streets: Von Kármán, "Über den Mechanismus des Widerstandes"