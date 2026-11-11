# EcoSim

EcoSim is a C++17 simulated ecosystem with procedural terrain, plants,
herbivores, and predators. Animals carry a sight gene, a neutral colour gene, and
XX/XY sex chromosomes. The simulation keeps a permanent lineage record and
exposes a Dear ImGui interface for inspecting behaviour, genetics, and
population trends.

## Features

- Seeded gradient-noise terrain with fBm, ridges, lakes, sandy shores, green
  highlands, and fog.
- Animated water with moving ripples and grazing-angle opacity.
- Plant growth and spread plus herbivore and predator behaviour.
- Mendelian inheritance, sight and colour mutation, reproduction, and ageing.
- Permanent lineage records for every animal ever born.
- Lineage tree, Punnett-square genetics view, and ImPlot population charts.
- Live simulation settings and restart-time population settings.
- Frame diagnostics (draw calls, simulation ms, render ms), stress mode, and
  one-frame PNG screenshots with a built-in PNG writer.

## Controls

Keyboard:

- `Space` - pause or resume
- `,` / `.` - halve / double simulation speed
- `R` - reset the current seed
- `N` - restart with the next seed
- `F` - toggle following the selected animal
- `WASD` or arrow keys - move the orbit camera
- `Left Shift` - faster camera movement
- `Esc` - quit

Mouse:

- Left-click an organism - select it
- Left-drag - orbit the camera
- Right-drag or middle-drag - pan
- Wheel - zoom
- Clicking an ImGui panel or plot does not move the camera

UI windows:

- **Simulation** - play/pause, reset, new seed, speed, follow, sight ring, and a
  Settings section. Live sliders change the current run; initial populations
  apply on reset/new seed.
- **HUD** - time, generation, populations, FPS, draw calls, and frame timings.
- **Selected organism** - identity, energy, state, pregnancy, genes, and family.
- **Lineage** - bounded tree of parents, grandparents, the selected animal,
  children, and grandchildren. Hover a node for details; click a living node to
  select it.
- **Genetics of this animal** - parental colour alleles and a highlighted Punnett
  square, the sight cross, and mutation warnings.
- **Statistics** - population, sight, current-sight histogram, and neutral
  colour-allele frequency charts.

## Editing the simulation

- Main parameters are adjusted in the 'Settings' tab of the Simulation window
- The full list of editable values is found at src/sim/SimConfig.h
- To change the terrain/world layout, change the seed value in src/world/Terrain.h

## Command line

```text
ecosim [seed]
ecosim [seed] --stress
ecosim [seed] --screenshot out.png [--sim-seconds S]
```

- `seed` selects the simulation run. The terrain seed is
  `TerrainConfig::seed` in `src/world/Terrain.h`.
- `--stress` starts with 800 plants, 150 herbivores, and 40 predators for
  measuring the worst-case frame cost.
- `--screenshot out.png` creates a hidden window, advances `S` simulated seconds
  (default 120) without rendering, renders one frame including the UI, writes a
  PNG, and exits with code 1 if the image has fewer than 50 distinct colours.
- Screenshot mode still needs a graphics display and OpenGL context. On a
  headless Linux machine, run it under a display server such as
  `xvfb-run -a ./ecosim 1 --screenshot out.png`.
- `ecosim_viewer` is the standalone terrain/water showcase. Use the same camera
  controls; it has no simulation or ImGui windows.

## Build

Requirements: CMake 3.20 or newer, a C++17 compiler, and network access for the
pinned GLFW, GLM, doctest, ImGui, and ImPlot FetchContent downloads.

### Windows with MinGW and Ninja

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

### Windows with Visual Studio 2022

```powershell
cmake -S . -B build-msvc -G "Visual Studio 17 2022" -A x64
cmake --build build-msvc --config Release
ctest --test-dir build-msvc -C Release --output-on-failure
```

### Linux

Install the compiler, CMake, Ninja, and OpenGL/X11/Wayland development packages:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build libgl1-mesa-dev \
    xorg-dev libwayland-dev libxkbcommon-dev
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

### Headless build

Disable the OpenGL viewer, ImGui, and screenshot application while keeping the
simulation, tools, and tests:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DECOSIM_BUILD_VIEWER=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

The CMake `POST_BUILD` step copies `assets/` next to `ecosim` and
`ecosim_viewer`, so launch those executables from their build directory or keep
the assets copied alongside them.

## Tests

CTest registers:

- `genetics`, `sim`, `behaviour`, `reproduction`, `world`, `view`, `ui`, `png`
- `balance_smoke` - runs two 300-second rescue-enabled seeds and fails if either
  ends without both animal kinds.

## Troubleshooting

- **CMake cannot find GLFW/OpenGL headers on Linux** - install the development
  packages listed above.
- **Screenshot mode fails or writes a blank image** - run it under `xvfb-run` (or
  another display server). The hidden window still needs a working GL context.
- **The application cannot find shaders or textures** - the `assets/` directory
  must be next to the executable. Re-run the build or copy `assets/` manually.
- **FetchContent fails** - check network access and Git; all third-party
  dependencies are pinned in `CMakeLists.txt`.
