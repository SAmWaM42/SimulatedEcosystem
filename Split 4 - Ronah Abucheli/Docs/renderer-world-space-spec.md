# 3D World-Space Renderer Specification

## Objective

Create a real 3D renderer that can place objects in world space, view them through a camera, and draw them from the perspective of that camera. The implementation should support:

- world-space object placement
- camera transform and view matrix
- perspective projection
- depth-aware rendering
- a simple scene graph or object collection
- a render loop that updates and draws each frame

This project should evolve from the current prototype into a proper camera-based 3D rendering pipeline rather than a framebuffer-only rasterizer.

## Current assessment

The current codebase contains a prototype software renderer, but it is not yet aligned with the required outcome.

The most important gaps are:

- no true camera class with projection logic
- no real model-view-projection pipeline
- no reusable scene or object world-space transform system
- no mesh abstraction for 3D objects
- no general-purpose 3D drawing path beyond a fixed software framebuffer test case
- no clear separation between world data, camera logic, and rendering logic

The current project in `ecosystem.c++` and `Components/unique.*` demonstrates a partial vector/polygon rendering approach, but it is still closer to a custom rasterization demo than a 3D world renderer.

## Required project outcome

The final implementation should support the following behavior:

1. Define world-space positions for 3D objects
2. Apply local transforms to object geometry (translation, rotation, scaling)
3. Convert object coordinates into world space
4. Move the camera in world space
5. Transform world coordinates into camera/view space
6. Project camera-space points using perspective projection
7. Clip and rasterize visible geometry
8. Render with depth testing and proper painter's order or z-buffer logic
9. Display the resulting frame in a window via OpenGL

## Project architecture target

### Core modules

- `Components/`:
  - keep low-level math and reusable data types here
  - move camera, transform, and mesh-related structures out of ad hoc code
- `Geometry/`:
  - mesh generation and primitive creation
- `Rendering/`:
  - renderer class, render pass management, projection pipeline, framebuffer upload
- `Managers/`:
  - scene manager, object list, update calls, draw calls
- `Entities/`:
  - world object definitions, game/entity wrappers
- `ecosystem.c++`:
  - application bootstrap, window setup, game loop, final integration point

### Data model target

- `Transform`
  - position, rotation, scale
- `Camera`
  - position, yaw, pitch, roll
  - view matrix
  - projection matrix
  - near/far plane
  - aspect ratio
- `Mesh`
  - vertices, indices, normals, UVs (if needed)
- `Entity` / `Renderable`
  - mesh reference
  - transform
  - color/material data
- `Scene`
  - list of renderables
  - active camera
  - world update loop

## Required implementation steps

### Step 1: Replace ad hoc math with a coherent transform system

The current `Transformer` and `Camera` definitions are not enough for a real renderer.

Required changes:

- add a proper 3D math layer
- use a matrix-based transform system (`glm` or custom matrices)
- separate local transform, world transform, view transform, projection transform
- store the camera as a transform plus view/projection matrices

Recommended decision:

- use `GLM` for robust matrix/vector operations unless the project explicitly requires a custom implementation

This is the cleanest path toward a correct MVP.

### Step 2: Introduce a proper camera model

Current camera code in `Components/unique.h` only subtracts camera position and applies inverse rotation.

The new camera must include:

- position in world space
- orientation (yaw/pitch/roll or quaternion)
- view matrix (`lookAt` style)
- projection matrix (`perspective` style)
- field-of-view, aspect ratio, near/far plane

This must be used in the render pipeline instead of custom ad hoc projection.

### Step 3: Convert the current shape model into a mesh-based representation

The current `shape` concept is a polygon wrapper with edge drawing and fill logic. That is not sufficient for general 3D objects.

The project should replace or wrap that with:

- `Mesh` storing vertices and indices
- `Renderable` referencing a mesh and its transform
- primitive generators for cube, pyramid, plane, and terrain patches

A single object should be able to be instantiated from a mesh and transformed in world space.

### Step 4: Build a complete world-space pipeline

The render pipeline should operate like this:

1. local vertex -> model transform
2. model -> world transform
3. world -> view transform (camera)
4. view -> projection transform
5. perspective divide
6. viewport transform
7. rasterization / fragment generation
8. depth test
9. final framebuffer or OpenGL draw

This is the critical missing architecture in the current project.

### Step 5: Add a proper scene manager

Add a `Scene` or `SceneManager` that tracks:

- all renderable objects
- the active camera
- object transforms over time
- update and render ordering

This is required to support future ecosystem objects instead of drawing one test polygon in isolation.

### Step 6: Implement separation of concerns between software and OpenGL display

The current project renders a generated framebuffer to a fullscreen quad using OpenGL. That is acceptable as a temporary display strategy, but it should not become the entire renderer.

A proper architecture is:

- geometry and transforms computed in C++
- framebuffer or fragment values generated by renderer
- OpenGL used only to display the final buffer or final triangles

The renderer should not be bound to a one-off demonstration object.

### Step 7: Add depth, clipping, and visibility rules

The current code does some line clipping and simple z-depth comparison, but it is not general enough.

Needed improvements:

- near/far plane clipping
- triangle clipping or polygon clipping logic
- z-buffer or painter's algorithm for visibility
- proper handling of backfaces or face culling

### Step 8: Add camera controls and object transforms in the main loop

The main loop should be able to:

- move the camera with keyboard input
- rotate the camera with mouse or keys
- update object transforms per frame
- redraw at a consistent frame rate

This is required before the scene can be treated as a true 3D view.

### Step 9: Validate with simple 3D test cases

Before expanding to ecosystem objects, verify correctness with:

- single triangle in front of camera
- cube with camera movement
- translated/rotated object in world space
- depth ordering between near and far triangles

This ensures the renderer is correct before more complex entities are added.

## File-specific change plan

### `ecosystem.c++`

Planned adjustments:

- remove the one-off test shape rendering logic from the main path
- build a scene with one or more 3D objects
- initialize the camera with a proper view/projection matrix
- add a main loop with update and render calls
- use consistent rendering calls instead of ad hoc framebuffer generation

### `Components/unique.h`

Planned adjustments:

- replace the current fixed-size `Viewport`/`pixel` design with a more general rendering state
- define transform/camera/mesh interfaces in a reusable way
- remove the current assumptions that everything is a single polygon test case
- add matrix/vector declarations and associated utility functions

### `Components/unique.c++`

Planned adjustments:

- remove or rewrite the current `line::set_edge_points` logic as part of a general mesh rasterizer
- replace `shape::set_fiiled_shape` with a real triangle/mesh rasterization pipeline
- fix the fill-depth bug where the z value used during fill is not correctly assigned
- convert camera view logic into proper matrix transforms
- stop treating the code as a single-screen framebuffer demo

### `CMakeLists.txt`

Planned adjustments:

- add `glm` include path if using GLM
- maintain a project structure with separate render/scene modules
- add any new source files as they are introduced

### New files to add

Suggested additions:

- `Rendering/Renderer.h` and `Rendering/Renderer.cpp`
- `Rendering/Camera.h` and `Rendering/Camera.cpp`
- `Geometry/Mesh.h` and `Geometry/Mesh.cpp`
- `Entities/Entity.h` and `Entities/Entity.cpp`
- `Managers/SceneManager.h` and `Managers/SceneManager.cpp`

These files would make the codebase match the intended architecture instead of keeping everything in a single ad hoc file.

## Recommended implementation order

1. add GLM or custom math layer
2. replace ad hoc `Transformer` with proper matrices
3. implement camera view/projection
4. build mesh representation
5. add world-space object transforms
6. add scene manager
7. validate with simple 3D shapes
8. add ecosystem entities and rendering on top

## Open questions

No blocking uncertainties remain for the high-level project specification. The missing decisions are implementation choices, not core requirements.

If the project later needs a concrete direction, the next choices would be:

- whether to use `GLM` or a custom matrix stack
- whether the renderer should stay software-rasterized or move mostly to OpenGL triangle rendering
- whether camera controls should be orbit-style, FPS-style, or a fixed cinematic camera

These decisions do not block the initial architectural plan.

## Final goal statement

The expected outcome is a project architecture where:

- objects exist in world space
- the camera exists in world space and observes those objects
- the world-to-camera and camera-to-projection transforms are explicit and correct
- the renderer draws what the camera sees
- the rendering loop supports a real 3D scene, not just a static demo framebuffer

That is the minimum required alignment with the intended “draw 3D objects in world space from the camera perspective” goal.
