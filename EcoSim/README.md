# EcoSim

EcoSim is a university computer-graphics project: a 3D ecosystem simulation
with plants, herbivores and predators that reproduce using simplified genetics.
It stays headless; this split adds a static terrain/water and primitive viewer.

## Build and run

### Windows with MSVC

```powershell
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
.\build\Release\ecosim_viewer.exe 1
```

### Windows with MSYS2/MinGW

From an MSYS2 MinGW shell with the UCRT64 toolchain installed:

```bash
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/ecosim_viewer.exe 1
```

### Linux

Required Debian/Ubuntu packages:

```bash
sudo apt install build-essential cmake ninja-build libgl1-mesa-dev xorg-dev libwayland-dev libxkbcommon-dev
```

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
./build/ecosim_viewer 1
```

The viewer copies `assets/` next to the executable after building. It resolves
assets relative to the executable, so launch it by path rather than through a
PATH lookup.

## Viewer controls

- Left-drag: orbit the camera
- Right-drag or middle-drag: pan the camera target
- Mouse wheel: zoom
- W/A/S/D or arrow keys: move the camera target
- Left Shift: move faster
- Escape: quit

## Folder layout

`src/genetics` and `src/sim` hold the graphics-free simulation and tests;
`src/world` contains the GL-free terrain, and `src/render` contains the mesh
factory, camera and asset helpers alongside the small GLFW/GLAD/OpenGL wrappers.
`apps` contains the console tools and the viewer, `tests` contains the doctest
suites, `assets/shaders` contains the GLSL programs, and `third_party/glad` holds
the generated OpenGL 3.3 core loader.

## Building without graphics

Turn the viewer off when only the headless simulation and its tests are needed:

```bash
cmake -S . -B build -DECOSIM_BUILD_VIEWER=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

This builds and tests the genetics, simulation, behaviour, reproduction and
world code without requiring GLFW, GLAD or an OpenGL driver.

