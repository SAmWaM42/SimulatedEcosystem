# Simulated Ecosystem

A C++ project focused on building a lightweight 3D simulation environment where objects can be placed in world space, observed from a camera, and later extended into an ecosystem of entities and interactions.

---

## Project Description

This project is being developed as a simulation and rendering foundation for an ecosystem-style application. The current focus is on creating a functional 3D camera/view pipeline, world-space object placement, and a software renderer that can draw primitive geometry in a scene before adding higher-level simulation logic such as organisms, behaviours, and interactions.

The aim is to establish a stable graphics foundation that can later support entity spawning, movement, interaction rules, and eventual UI overlays for observing the system.

---

## Group Members and Responsibilities

| Member | Responsibility |
|---|---|
| Samuel Wamae | 3D scene construction, object placement, and geometry representation |
| Philip Gakuo| Camera system, world-to-camera transforms, and projection logic |
| Njihia Muranga| Rendering pipeline, software rasterization, and line/triangle drawing |
| Philip Leo | Ecosystem entity modelling and simulation structure |
| Kelvin Maina | User interaction, controls, rendering validation, and testing |
| Ronah Abucheli | Integration, documentation, build management, and project coordination |

All group members contribute to testing, debugging, and final integration.

---

## Technologies and Tools

- C++
- CMake
- GLFW for window creation and input handling
- OpenGL for window/display presentation only
- Custom software rendering pipeline for world-space scene drawing
- Standard C++ math and vector utilities for transforms and projection

---

## Graphics Techniques and Algorithms Implemented So Far

The current implementation includes the following graphics and rendering features:

- Basic window creation and OpenGL context setup with GLFW
- Simple 3D mesh generation for primitive objects such as cubes
- World-space transform pipeline using position, rotation, and scale
- Camera-based world-to-camera transformation logic
- Perspective projection into screen space
- Software framebuffer rendering with a depth buffer
- Bresenham-style line drawing for primitive edge rendering
- Triangle scanline filling using barycentric interpolation for solid surface coverage
- Depth testing to ensure nearest visible fragments are drawn over farther ones
- Simple camera movement and rotation controls for exploratory testing

This is a foundational renderer for visualizing objects in a 3D scene rather than a complete OpenGL scene graph.

---

## How to Compile and Run the Current Version

### Prerequisites

- **C++17 compiler**: MinGW GCC (g++ in `C:\MinGW\bin\`)
- **CMake 3.16+** (tested with 4.2.3)
- **GLFW**: Automatically fetched and built from source via CMake `FetchContent` (no manual install needed)
- **OpenGL**: Windows system library (`opengl32`) — included with MinGW

### Option A: Using VS Code

1. Install the recommended VS Code extensions: **CMake Tools**, **C/C++** (by Microsoft)
2. Open the project folder in VS Code
3. Select the compiler when prompted (or via **CMake: Select a kit** → `GCC`)
4. To configure + build: run the **Terminal → Run Build Task** (`Ctrl+Shift+B`) and select **Build**
5. To run: execute the **Run** task, or press `F5` to launch with the debugger



### Option B: In the Command Line (Git Bash / MSYS2)
```bash
./build.sh build      # configures with MinGW Makefiles and builds
./build.sh run        # runs the executable
```

Or manually:

```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
./build/program.exe
```

---

## Current State

The project is currently in a renderer prototype stage. The system is capable of creating a simple 3D scene, positioning objects in world space, moving and rotating a camera, and drawing primitives through a custom software rendering path. The goal is to extend this base into a broader ecosystem simulation with multiple entities and interactions.

---

## Notes

This README reflects the current implementation status and will continue to be updated as the project grows.
