# EcoSim architecture

EcoSim keeps the simulation independent from the window and UI. Most modules are
plain C++17 libraries; only the render and ImGui layers touch OpenGL.

## Module map

- `src/core` - `Rng`, the single seeded random stream used by terrain, genetics,
  and behaviour. GL-free.
- `src/genetics` - `Genome`, gamete formation, fertilisation, mutation, and trait
  expression. GL-free (`eco_genetics`).
- `src/sim` - `SimConfig`, `World`, `SpatialGrid`, `Organism`, `Simulation`,
  reproduction, `LineageTracker`, `StatsManager`, and `SimClock`. GL-free
  (`eco_sim`), depending on genetics and GLM.
- `src/world` - the procedural `Terrain`. GL-free (`eco_world`, which also holds
  GL-free mesh/camera/asset helpers).
- `src/render` - GL-free camera, mesh construction, picking, palette, and asset
  helpers live in `eco_world`/`eco_view`; the OpenGL `Window`, `Shader`, `Mesh`,
  `DebugDraw`, and `SimRenderer` live in `eco_render`.
- `src/ui` - `UiText`, `ChartData`, `LineageLayout`, and `Punnett` are GL-free and
  compiled into `eco_view`. `UiManager` and `Panels` use Dear ImGui/ImPlot and
  are compiled into the viewer-only `eco_ui`.
- `src/tools` - the dependency-free PNG writer (`eco_tools`). GL-free.
- `apps` - `ecosim` (interactive and screenshot), `ecosim_viewer`,
  `sim_headless`, `balance_run`, and `genetics_demo`.
- `tests` - doctest suites for genetics, simulation, behaviour, reproduction,
  world, view, UI data, PNG, and the balance smoke test.

Dependency direction:

```text
core -> genetics -> sim -> eco_view -> eco_ui -> ecosim
                         \-> eco_render -> ecosim
eco_tools -> ecosim / png_tests
world -> sim, eco_view, eco_render
```

`eco_world` supplies terrain and GL-free render data to `eco_sim`,
`eco_view`, and `eco_render`. The GL-free libraries are `eco_genetics`,
`eco_sim`, `eco_world`, `eco_view`, and `eco_tools`; `eco_render` requires
OpenGL and `eco_ui` requires ImGui/ImPlot.

## Per-frame flow

1. GLFW events are polled into `InputState`.
2. The ImGui frame begins; panels are submitted and return `UiActions`.
3. UI actions are applied, then `SimClock` advances zero or more fixed steps.
4. The camera updates and 3D picking runs if the click was not consumed by UI.
5. Terrain, organisms, debug lines, and water are rendered with fog.
6. ImGui draw data is rendered last and the buffers are swapped.

Screenshot mode runs the fixed steps without rendering, creates one normal
rendered frame including the UI, reads the framebuffer, flips it top-down, writes
a PNG, and checks that the image is not blank.

## Adding a gene

1. Add the allele/trait representation and any rate/range to
   `GeneticsConfig`.
2. Extend `Genome`, `Gamete`, `randomGenome`, `makeGamete`, `fertilise`, and
   `cross`, preserving maternal slot 0 and paternal slot 1.
3. Add expression and clamping rules and expose them through `Genetics.h`.
4. Use the expressed trait in `Animal` or behaviour, add it to lineage/UI data,
   update the docs and add property-based tests.

## Adding a behaviour state

1. Add the enumerator to `State` in `src/sim/Organism.h`.
2. Implement its transition and action in `Animal::think` and the relevant
   movement/feeding/reproduction helpers.
3. Add animation mapping in `SimRenderer::gaitForState`.
4. Add its display name in `UiText::stateName`.
5. Add tests for transitions and invariants, then update the docs.
