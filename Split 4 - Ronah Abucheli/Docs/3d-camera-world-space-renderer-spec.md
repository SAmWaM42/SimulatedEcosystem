# 3D Camera World-Space Renderer Specification

## Goal

Create a software-rendered 3D scene where objects exist in world space, can be added to the scene, and are drawn from the perspective of a camera using a proper world-to-camera-to-screen projection pipeline. GLFW may be used only for window creation, event polling, and input handling. Rendering itself must not rely on OpenGL for drawing the scene. The renderer should use software rasterization techniques, including point clipping and Bresenham-style line drawing.

This specification documents the required code changes to reach that outcome.

## Scope

The project must support:

- 3D object definitions in world space
- layering/scene management of many objects
- a camera transform with position and orientation
- conversion from world space to camera space
- projection to screen space
- clipping of points/lines against the view frustum
- Bresenham-based line rasterization
- polygon/mesh drawing using software rasterization
- user input for camera movement and object placement
- a display loop using a framebuffer or equivalent software rendering surface

## Non-goals for this stage

- no OpenGL-based triangle pipeline for scene rendering
- no immediate full shading/lighting system
- no complex texture mapping
- no advanced physically-based camera model
- no ECS framework requirement

## Current project assessment

The current project has a partially structured transform and matrix system, but it is not yet aligned with the required architecture. The main missing elements are:

- no full scene model with world-space object registration
- no renderer class that consumes camera + object data and produces pixels
- no real camera-space projection path for 3D objects
- no robust clipping pipeline against the view volume
- no consistent software rasterization path for polygons or meshes
- no world-space object management beyond prototype shape logic
- no explicit separation between geometry, rendering, camera, and scene data

## Required architecture

### 1. World-space object model

Add or refactor the code to define objects as data in world space.

Objects should have:

- a unique identifier
- a local vertex list or mesh definition
- a transform: position, rotation, scale
- a color or material descriptor
- optional visibility flag

Recommended design:

- `Geometry/mesh.h` or equivalent: mesh definitions and primitive builders
- `Entities/entity.h` or equivalent: object wrapper with world transform
- `Managers/scene_manager.h` or equivalent: object collection and update loop

### 2. Camera model

The camera must be independent from the render loop and hold the following:

- position in world space
- yaw, pitch, roll or equivalent rotation
- forward, right, up vectors
- view matrix or equivalent world-to-camera transform
- projection parameters (FOV, aspect ratio, near/far planes)

The camera should be usable for the following conversions:

1. world point -> camera space
2. camera point -> projected screen space
3. clipping against the view volume

### 3. View and projection pipeline

The renderer should define a structured pipeline like this:

1. object local vertices
2. object world transform
3. camera view transform
4. perspective divide / projection to screen space
5. viewport mapping
6. clipping against screen bounds
7. line/polygon rasterization
8. depth testing and pixel writing

This must be explicit and reusable; it should not be hard-coded into a single test loop.

### 4. Software rasterization

Rendering must use software rasterization rather than OpenGL drawing. That means the scene must be converted into pixel samples in the framebuffer.

The render pipeline should include:

- framebuffer data structure (RGB + depth buffer)
- clear operation
- z-buffer or depth buffer support
- point clipping and line clipping against the viewport
- Bresenham-based line algorithm for rasterization
- polygon fill or scanline fill for solid surfaces

### 5. Object addition to the scene

There must be explicit APIs to add objects to the scene, for example:

- add_triangle
- add_quad
- add_cube
- add_mesh
- add_object

These should register world-space geometry and immediately be included in the render loop.

## Requirements by subsystem

### Geometry layer

The geometry layer should support:

- `Vec3` / `Vec2` point types
- mesh data: vertices + faces/edges
- primitives: cube, pyramid, plane, axis-aligned box, triangle
- transformation support: translate, rotate, scale

Required modifications:

- replace or refactor the current `Shape` concept so it is mesh-driven
- add object-specific world transforms separate from local mesh data
- add a reusable mesh builder for simple geometry

### Camera + transforms

The transform layer must be capable of:

- rotate around X, Y, Z
- translate in 3D
- build view matrix from camera position/orientation
- apply world transform to object vertices
- convert camera-space coordinates to screen-space coordinates

The current transform matrix style may remain as a custom implementation, but it must be made consistent and fully used by the rendering flow.

### Clipping system

Point clipping and line clipping are required.

Required functions should include:

- point-in-view test
- line segment clipping against the view rectangle or view volume
- rejection of points behind the camera
- handling of near-plane clipping

This is necessary to keep rendered geometry within the view and prevent invalid screen coordinates.

### Line rasterization

Bresenham’s algorithm is required for line drawing.

The implementation should:

- rasterize integer screen-space line segments
- write candidate pixels into the framebuffer
- handle both X-major and Y-major cases
- support depth comparisons per pixel

This should be a core rendering utility rather than a one-off function inserted inside an object drawing routine.

### Renderer and framebuffer

A proper renderer should include:

- framebuffer width/height
- clear color and clear depth
- list of objects to render
- active camera
- software draw loop per frame
- optional per-object sorting by depth

The renderer should not use OpenGL for drawing the actual scene. It should produce pixels directly to a software framebuffer and then display them through a simple GLFW-backed window if desired.

## Concrete code changes required

### 1. Replace or extend `Geometry/primitives.h` and `Geometry/primitives.c++`

These files should be expanded to include:

- mesh definitions
- face definitions or edge lists
- world transformation logic
- 3D primitive creation helpers

The current matrix/transform work should be integrated here rather than left as exploratory tests.

### 2. Add a dedicated camera module

Add a file such as:

- `Rendering/camera.h`
- `Rendering/camera.c++`

This should contain:

- camera state
- view matrix calculation
- projection parameters
- world-to-camera transform
- screen projection helpers

### 3. Add a dedicated scene manager

Add a file such as:

- `Managers/scene_manager.h`
- `Managers/scene_manager.c++`

This should contain:

- list of objects in the scene
- active camera reference
- add/remove object functions
- update loop
- render call scheduling

This is what will allow multiple objects to co-exist in a 3D space.

### 4. Add a dedicated rendering class

Add a file such as:

- `Rendering/renderer.h`
- `Rendering/renderer.c++`

This should contain:

- framebuffer state and depth buffer
- camera reference
- draw call dispatch
- line and polygon rasterization routines
- clipping utilities
- final pixel output to a window or software buffer

The renderer must own the drawing operations. It should not live only inside the test app.

### 5. Update the main application entry point

`ecosystem.c++` should be reorganized so that it does the following:

- initialize GLFW window
- initialize a software framebuffer or pixel buffer
- construct a scene
- add multiple 3D objects to the scene
- create a camera with position/orientation
- run a frame loop
- render visible geometry from the camera perspective
- handle keyboard input for movement and rotation
- update display on each frame

The app should not be a single test function. It should become a real scene viewer.

### 6. Add viewport/clip logic

Required code functionality:

- convert projected screen coordinates to viewport coordinates
- reject out-of-range points
- clip line segments to viewport bounds
- handle near-plane clipping for points behind the camera

This should be implemented in a reusable helper module rather than ad hoc inside object draw code.

### 7. Add Bresenham line drawing

A dedicated line rasterizer should be implemented for 3D line segments after projection to screen space.

It should:

- accept two screen-space endpoints
- compute integer stepping values
- draw along the line using integer arithmetic
- write color + depth to framebuffer

This should be used for edge drawing and/or polygon outlines.

### 8. Add polygon rendering support

Once line drawing is functional, add polygon drawing logic for triangles/quads.

Requirements:

- project polygon vertices to screen space
- clip the polygon if necessary
- fill the polygon using scanline or edge-walking approach
- respect depth buffer values

This may be implemented as triangle rasterization first, then extended to general polygons.

### 9. Update the test harness

The test files should be rewritten so that they validate the actual required system:

- transform correctness
- camera view transform correctness
- projection math
- clipping behavior
- Bresenham line drawing
- object list and scene management

The current tests in `Tests/tests.h` are useful as a starting point but should be rebuilt around the final software-rendering target.

## Implementation ordering

The implementation should proceed in this order:

1. establish stable `Vec2`/`Vec3` and transform math
2. implement camera state and world-to-camera transform
3. implement projection to screen space
4. implement clipping utilities
5. implement Bresenham line drawing
6. implement polygon/triangle rasterization
7. implement scene object list
8. add multiple object placement in world space
9. add camera controls and render loop
10. validate with a simple cube and then multi-object scene

## Key design decisions

### Software rendering only

The scene must be rendered via software rasterization. OpenGL must not be used as the renderer, though it may still be used for a fullscreen software texture display if the project later wants to show the framebuffer in a window.

### GLFW usage

GLFW is allowed for:

- creating the window
- handling keyboard/mouse input
- event polling
- optional presentation of the software buffer

GLFW is not allowed to be the actual rendering engine for the 3D geometry.

### Camera control

The camera should allow:

- forward/back movement
- strafing left/right
- vertical movement if needed
- yaw/pitch rotation

This should be implemented through keyboard input and frame-based update calculations.

## Minimal acceptable milestone

The project should be considered aligned with the expected outcome once it supports the following:

- at least one object placed in world space
- a camera with position and look direction
- view transform from world to camera space
- projection to screen space
- point clipping and line clipping
- Bresenham line rasterization
- visible object edges drawn from camera perspective
- multiple objects in a shared scene
- a frame loop that updates and redraws the scene

## Risks and known pitfalls

- mixing OpenGL render calls with software rasterization will blur the design
- building a general camera without clear world-space conventions causes incorrect transforms
- clipping logic must be implemented before final rasterization or it will produce invalid pixels
- line drawing and polygon fill should support integer coordinates for stable pixel placement
- depth testing must be explicit and not hidden in a one-off function

## Final expected outcome

The final project should be a software-rendered 3D scene viewer in which:

- objects exist in world space
- object transforms are applied before rendering
- the camera moves independently in world space
- the scene is drawn from the camera’s perspective
- point clipping and Bresenham line drawing are used in the rendering pipeline
- multiple objects are added to the world and shown according to the camera view

This is the required implementation target for the project.
