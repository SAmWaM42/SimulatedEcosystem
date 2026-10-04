# Simulated 3D Ecosystem

### An Interactive Visualization of Ecosystem Interactions and Genetic Inheritance

---

## Description

Simulated 3D Ecosystem is an interactive 3D application that models a simplified virtual
ecosystem of plants, herbivores, and predators. Organisms move, feed, compete for
resources, and reproduce, passing simplified genetic traits — such as size, speed, colour,
and energy efficiency — to their offspring, with occasional random mutation.

The project is intended as an educational visualization and experimentation tool for
exploring ecosystem interactions (feeding, predation, competition) and genetic inheritance
across multiple generations, rather than as a scientifically accurate biological model.

---

## Group Members & Responsibilities

| Member | Responsibility |
|---|---|
| Member 1 | 3D environment, terrain, scene construction, and graphical rendering |
| Member 2 | Organism modelling, spawning, movement, and animation |
| Member 3 | Ecosystem behaviour, interactions, collision detection, and organism states |
| Member 4 | Genetic representation, inheritance, reproduction, and mutation |
| Member 5 | User interface, camera controls, organism selection, and information display |
| Member 6 | Integration, testing, performance optimization, documentation, and coordination |

All members also share in integration, debugging, testing, and documentation.

---

## Technologies / Tools

- **C++** — core application language
- **OpenGL** — real-time 3D rendering
- **CMake** — build system / project configuration

*(Additional libraries — e.g. windowing, math, model loading — to be listed here as they are
added to the project.)*

---

## Graphics Techniques / Algorithms Implemented So Far

Development is currently focused on getting primary rendering working. Implemented so far:

- Basic OpenGL window and rendering context setup
- Core 3D geometry rendering (meshes/primitives for terrain and placeholder organism shapes)
- Basic transformations (translation, rotation, scaling) for positioning objects in the scene
- An initial 3D camera system for viewing the scene

Not yet implemented: lighting/shading, organism animation, collision detection, genetic
inheritance, and the user interface — these are planned for upcoming development phases.

---

## How to Compile and Run

The project is built using **CMake**.

```bash
# from the project root
mkdir build
cd build
cmake ..
cmake --build .
```

Once the build completes, the runnable executable will be located in the `build/`
directory. Run it directly from there, e.g.:

```bash
./build/program
```
This process has been automated in a build script
to build/update the `build` DIR:
```bash
./build.sh build
```
to run the executable:
```bash
./build.sh run
```

---

## Current Progress

Primary rendering is currently being handled. 

---

## Known Limitations / Issues

- No organism behaviour yet (feeding, predation, reproduction, competition are not
  implemented).
- No genetic representation, inheritance, or mutation implemented yet.
- No collision detection or distance-based interaction checks yet.
- No animation system — organisms are currently static placeholders.
- No user interface (simulation controls, organism selection, or information display).
- No lighting/shading applied yet — current rendering is primitive/unshaded.
- Project is in an early stage; performance and stability have not yet been evaluated at
  scale.

---

*This README reflects the current (early) state of the project and will be updated as
development progresses.*