// Main interactive EcoSim application: runs the terrain-aware simulation,
// renders organisms and debug overlays, draws the ImGui UI, and handles camera,
// time, and input routing.
#include <algorithm>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "render/Assets.h"
#include "render/Camera.h"
#include "render/DebugDraw.h"
#include "render/Mesh.h"
#include "render/MeshFactory.h"
#include "render/Picking.h"
#include "render/Shader.h"
#include "render/SimRenderer.h"
#include "render/Window.h"
#include "sim/SimClock.h"
#include "sim/SimConfig.h"
#include "sim/Simulation.h"
#include "sim/Stats.h"
#include "ui/Panels.h"
#include "ui/UiManager.h"
#include "world/Terrain.h"

namespace {
// Interpolates a render position between the previous and current fixed step.
glm::vec2 interpolatedPosition(const Organism& organism, float alpha) {
    return organism.prevPosition +
           (organism.position - organism.prevPosition) * alpha;
}

// Builds the compact HUD snapshot shown at the top-right of the window.
HudData makeHudData(const Simulation& sim, int fps, bool paused) {
    HudData hud;
    hud.time = sim.time();
    hud.plants = sim.count(Kind::Plant);
    hud.fps = fps;
    hud.paused = paused;

    const std::vector<StatsSample>& samples = sim.stats().samples();
    if (!samples.empty()) {
        hud.generation = samples.back().maxGeneration;
    }

    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        if (!organism->alive) {
            continue;
        }
        const Animal* animal = dynamic_cast<const Animal*>(organism.get());
        if (animal == nullptr) {
            continue;
        }
        const bool female = animal->sex == genetics::Sex::Female;
        if (animal->kind() == Kind::Herbivore) {
            if (female) {
                ++hud.herbivoreFemales;
            } else {
                ++hud.herbivoreMales;
            }
        } else if (animal->kind() == Kind::Predator) {
            if (female) {
                ++hud.predatorFemales;
            } else {
                ++hud.predatorMales;
            }
        }
    }
    return hud;
}
}  // namespace

int main(int argc, char** argv) {
    try {
        setExecutablePath(argv[0]);

        std::uint32_t seed = 1;
        if (argc > 1) {
            seed = static_cast<std::uint32_t>(std::stoul(argv[1]));
        }

        SimConfig runConfig;
        TerrainConfig terrainConfig;
        terrainConfig.size = runConfig.worldSize;
        terrainConfig.seed = seed;
        const Terrain terrain(terrainConfig);

        auto simulation =
            std::make_unique<Simulation>(runConfig, seed, &terrain);
        simulation->populate();

        Window window(1280, 720, "EcoSim");
        window.enableDepthAndCull();
        UiManager ui(window.handle());
        SimClock clock(runConfig.fixedDt);
        SimRenderer renderer;
        DebugDraw debugDraw;
        Camera camera(terrain);

        Shader litShader(assetPath("shaders/lit.vert"),
                         assetPath("shaders/lit.frag"));
        Shader terrainShader(assetPath("shaders/terrain.vert"),
                             assetPath("shaders/terrain.frag"));
        Shader waterShader(assetPath("shaders/water.vert"),
                           assetPath("shaders/water.frag"));
        Mesh terrainMesh(makeTerrainMesh(terrain));
        Mesh waterMesh(makePlane());

        std::cout << "EcoSim controls: Space pause/resume, ',' speed / 2, "
                     "'.' speed * 2, R restart, N next seed, F follow, "
                     "left-click select, left-drag orbit, right-drag pan, "
                     "wheel zoom, WASD move, Shift faster, ESC quit."
                  << std::endl;

        const glm::vec3 lightDirection =
            glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
        const float worldCentre = runConfig.worldSize * 0.5f;
        const float bodyRadius = runConfig.bodyRadius;

        std::uint32_t selectedId = 0;
        bool follow = false;
        bool showSightRing = true;
        double lastTime = window.time();
        int frameCount = 0;
        double fpsElapsed = 0.0;
        int fps = 0;

        const auto restart = [&](std::uint32_t newSeed) {
            seed = newSeed;
            simulation =
                std::make_unique<Simulation>(runConfig, seed, &terrain);
            simulation->populate();
            renderer.reset();
            selectedId = 0;
            follow = false;
        };

        const auto updateTitle = [&](int currentFps) {
            window.setTitle("EcoSim | FPS " + std::to_string(currentFps));
        };
        updateTitle(0);

        while (!window.shouldClose()) {
            window.pollEvents();
            ui.beginFrame();

            const bool uiMouse = ui.wantsMouse();
            const bool uiKeyboard = ui.wantsKeyboard();
            const double now = window.time();
            const double rawFrameDt = now - lastTime;
            const float frameDt = static_cast<float>(
                std::clamp(rawFrameDt, 0.0, 0.1));
            lastTime = now;
            InputState input = window.input();

            if (uiMouse) {
                input.mouseDelta = glm::vec2(0.f);
                input.scroll = 0.f;
                input.orbitDrag = false;
                input.panDrag = false;
                input.clicked = false;
            }
            if (uiKeyboard) {
                input.moveForward = false;
                input.moveBack = false;
                input.moveLeft = false;
                input.moveRight = false;
                input.fast = false;
            }

            if (!uiKeyboard) {
                if (window.keyPressed(GLFW_KEY_SPACE)) {
                    clock.setPaused(!clock.paused());
                }
                if (window.keyPressed(GLFW_KEY_COMMA)) {
                    clock.setSpeed(clock.speed() / 2.f);
                }
                if (window.keyPressed(GLFW_KEY_PERIOD)) {
                    clock.setSpeed(clock.speed() * 2.f);
                }
                if (window.keyPressed(GLFW_KEY_R)) {
                    restart(seed);
                }
                if (window.keyPressed(GLFW_KEY_N)) {
                    restart(seed + 1);
                }
                if (window.keyPressed(GLFW_KEY_F)) {
                    follow = (selectedId != 0) ? !follow : false;
                }
            }

            const HudData hud =
                makeHudData(*simulation, fps, clock.paused());
            UiActions uiActions;
            drawControlsPanel(clock, follow, selectedId != 0, showSightRing,
                              runConfig, *simulation, seed, uiActions);
            drawInspectorPanel(*simulation, selectedId, uiActions);
            drawStatisticsPanel(*simulation);
            drawHud(hud);

            if (uiActions.selectId != 0 &&
                simulation->find(uiActions.selectId) != nullptr) {
                selectedId = uiActions.selectId;
            }
            if (uiActions.toggleFollow) {
                follow = (selectedId != 0) ? !follow : false;
            }
            if (uiActions.togglePause) {
                clock.setPaused(!clock.paused());
            }
            if (uiActions.restart) {
                restart(seed);
            }
            if (uiActions.newSeed) {
                restart(seed + 1);
            }

            const int steps = clock.advance(frameDt);
            for (int i = 0; i < steps; ++i) {
                simulation->step();
            }
            const float alpha = clock.alpha();

            if (selectedId != 0 &&
                simulation->find(selectedId) == nullptr) {
                selectedId = 0;
                follow = false;
            }

            InputState cameraInput = input;
            if (follow && selectedId != 0) {
                const Organism* target = simulation->find(selectedId);
                if (target != nullptr) {
                    camera.followTarget(
                        interpolatedPosition(*target, alpha), frameDt);
                } else {
                    selectedId = 0;
                    follow = false;
                }
                cameraInput.moveForward = false;
                cameraInput.moveBack = false;
                cameraInput.moveLeft = false;
                cameraInput.moveRight = false;
                cameraInput.panDrag = false;
            }
            camera.update(cameraInput, frameDt, terrain);

            if (input.clicked) {
                const Ray ray = camera.screenRay(
                    input.clickPos, window.width(), window.height());
                selectedId = pickOrganism(ray, *simulation, terrain);
                if (selectedId == 0) {
                    follow = false;
                }
            }

            window.clear(0.53f, 0.81f, 0.92f);

            const glm::mat4 view = camera.viewMatrix();
            const glm::mat4 projection =
                camera.projectionMatrix(window.aspect());
            const glm::vec3 cameraPosition = camera.eye();

            terrainShader.use();
            terrainShader.setMat4("uModel", glm::mat4(1.f));
            terrainShader.setMat4("uView", view);
            terrainShader.setMat4("uProj", projection);
            terrainShader.setMat3("uNormalMatrix", glm::mat3(1.f));
            terrainShader.setVec3("uLightDir", lightDirection);
            terrainShader.setFloat("uWaterLevel", terrain.waterLevel());
            terrainShader.setFloat("uMaxHeight",
                                   terrain.config().maxHeight);
            terrainMesh.draw();

            litShader.use();
            litShader.setMat4("uView", view);
            litShader.setMat4("uProj", projection);
            litShader.setVec3("uLightDir", lightDirection);
            litShader.setVec3("uCameraPos", cameraPosition);
            renderer.draw(*simulation, terrain, litShader, alpha, frameDt,
                          selectedId);

            if (selectedId != 0) {
                const Organism* selected = simulation->find(selectedId);
                const Animal* animal =
                    dynamic_cast<const Animal*>(selected);
                if (animal != nullptr) {
                    const glm::vec2 position =
                        interpolatedPosition(*animal, alpha);
                    const float baseY =
                        terrain.heightAt(position.x, position.y);
                    const float lineY = baseY + 1.5f * bodyRadius;
                    if (showSightRing) {
                        debugDraw.circleOnTerrain(
                            position, animal->sight,
                            glm::vec3(1.f, 0.95f, 0.5f), terrain);
                    }

                    const auto addTargetLine =
                        [&](uint32_t targetId, glm::vec3 colour) {
                            if (targetId == 0) {
                                return;
                            }
                            const Organism* target =
                                simulation->find(targetId);
                            if (target == nullptr) {
                                return;
                            }
                            const float targetY =
                                terrain.heightAt(target->position.x,
                                                 target->position.y) +
                                1.5f * bodyRadius;
                            debugDraw.line(
                                glm::vec3(position.x, lineY, position.y),
                                glm::vec3(target->position.x, targetY,
                                          target->position.y),
                                colour);
                        };
                    addTargetLine(animal->foodId,
                                  glm::vec3(0.2f, 0.9f, 0.3f));
                    addTargetLine(animal->threatId,
                                  glm::vec3(1.f, 0.25f, 0.2f));
                    addTargetLine(animal->mateId,
                                  glm::vec3(0.9f, 0.3f, 0.9f));
                }
            }
            debugDraw.flush(view, projection);

            window.beginBlendPass();
            const glm::mat4 waterModel =
                glm::translate(glm::mat4(1.f),
                               glm::vec3(worldCentre, terrain.waterLevel(),
                                         worldCentre)) *
                glm::scale(glm::mat4(1.f),
                           glm::vec3(runConfig.worldSize, 1.f,
                                     runConfig.worldSize));

            waterShader.use();
            waterShader.setMat4("uModel", waterModel);
            waterShader.setMat4("uView", view);
            waterShader.setMat4("uProj", projection);
            waterShader.setMat3(
                "uNormalMatrix",
                glm::inverseTranspose(glm::mat3(waterModel)));
            waterShader.setVec3("uLightDir", lightDirection);
            waterShader.setVec3("uCameraPos", cameraPosition);
            waterMesh.draw();
            window.endBlendPass();

            ui.render();
            window.swapBuffers();

            ++frameCount;
            fpsElapsed += rawFrameDt;
            if (fpsElapsed >= 0.5) {
                fps = static_cast<int>(
                    static_cast<double>(frameCount) / fpsElapsed);
                updateTitle(fps);
                frameCount = 0;
                fpsElapsed = 0.0;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "EcoSim error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}
