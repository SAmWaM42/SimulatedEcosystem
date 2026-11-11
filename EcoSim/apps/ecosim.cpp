// Main EcoSim application: runs the simulation, renders terrain/organisms/water
// with fog, draws the ImGui UI, and supports stress and screenshot modes.
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <glad/gl.h>
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
#include "tools/PngWriter.h"
#include "ui/Panels.h"
#include "ui/UiManager.h"
#include "world/Terrain.h"

namespace {
constexpr float kFogDensity = 0.00012f;
constexpr float kEmaFactor = 0.05f;

// Command-line options for the interactive and screenshot modes.
struct AppOptions {
    std::uint32_t seed = 1;
    bool stress = false;
    std::string screenshotPath;
    double screenshotSeconds = 120.0;
};

// Parses the optional seed and the --stress/--screenshot/--sim-seconds flags.
AppOptions parseArguments(int argc, char** argv) {
    AppOptions options;
    bool seedSet = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--stress") {
            options.stress = true;
        } else if (argument == "--screenshot") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--screenshot requires a path");
            }
            options.screenshotPath = argv[++i];
        } else if (argument == "--sim-seconds") {
            if (i + 1 >= argc) {
                throw std::invalid_argument("--sim-seconds requires a value");
            }
            options.screenshotSeconds = std::stod(argv[++i]);
            if (options.screenshotSeconds < 0.0) {
                throw std::invalid_argument(
                    "--sim-seconds must be non-negative");
            }
        } else if (!argument.empty() && argument.front() != '-' && !seedSet) {
            options.seed =
                static_cast<std::uint32_t>(std::stoul(argument));
            seedSet = true;
        } else {
            throw std::invalid_argument("Unknown argument: " + argument);
        }
    }
    return options;
}

// Applies the worst-case starting population used by the stress benchmark.
void applyStress(SimConfig& config) {
    config.initialPlants = 800;
    config.initialHerbivores = 150;
    config.initialPredators = 40;
}

// Interpolates a render position between the previous and current fixed step.
glm::vec2 interpolatedPosition(const Organism& organism, float alpha) {
    return organism.prevPosition +
           (organism.position - organism.prevPosition) * alpha;
}

// Builds the compact HUD snapshot, including counters measured last frame.
HudData makeHudData(const Simulation& sim, int fps, bool paused,
                    int drawCalls, float simMs, float renderMs) {
    HudData hud;
    hud.time = sim.time();
    hud.plants = sim.count(Kind::Plant);
    hud.fps = fps;
    hud.drawCalls = drawCalls;
    hud.simMs = simMs;
    hud.renderMs = renderMs;
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
        const AppOptions options = parseArguments(argc, argv);
        std::uint32_t seed = options.seed;

        SimConfig runConfig;
        if (options.stress) {
            applyStress(runConfig);
        }

        // Terrain keeps its own seed from TerrainConfig; the command-line seed
        // controls the simulation so that editing Terrain.h changes the map.
        TerrainConfig terrainConfig;
        terrainConfig.size = runConfig.worldSize;
        const Terrain terrain(terrainConfig);

        auto simulation =
            std::make_unique<Simulation>(runConfig, seed, &terrain);
        simulation->populate();

        // Screenshot mode needs a display/GL context but keeps the window
        // hidden. On Linux this still requires a display, e.g. xvfb-run.
        const bool screenshotMode = !options.screenshotPath.empty();
        if (screenshotMode) {
            if (!glfwInit()) {
                throw std::runtime_error(
                    "Failed to initialize GLFW for screenshot mode");
            }
            glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        }

        Window window(1280, 720,
                      screenshotMode ? "EcoSim Screenshot" : "EcoSim");
        window.enableDepthAndCull();

        if (screenshotMode) {
            const long stepCount = std::lround(
                options.screenshotSeconds / runConfig.fixedDt);
            for (long i = 0; i < stepCount; ++i) {
                simulation->step();
            }
        }

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

        const glm::vec3 lightDirection =
            glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));
        const glm::vec3 fogColour(0.53f, 0.81f, 0.92f);
        const float worldCentre = runConfig.worldSize * 0.5f;
        const float bodyRadius = runConfig.bodyRadius;

        std::uint32_t selectedId = 0;
        bool follow = false;
        bool showSightRing = true;
        float simMsAverage = 0.f;
        float renderMsAverage = 0.f;
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

        // Draws the world, organisms, debug overlays, and water. The caller
        // times this along with the final UI draw for the render counter.
        const auto renderScene = [&](std::uint32_t selected, float alpha,
                                     float frameDt, bool sightRing) {
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
            terrainShader.setVec3("uCameraPos", cameraPosition);
            terrainShader.setFloat("uWaterLevel", terrain.waterLevel());
            terrainShader.setVec3("uFogColor", fogColour);
            terrainShader.setFloat("uFogDensity", kFogDensity);
            terrainMesh.draw();

            litShader.use();
            litShader.setMat4("uView", view);
            litShader.setMat4("uProj", projection);
            litShader.setVec3("uLightDir", lightDirection);
            litShader.setVec3("uCameraPos", cameraPosition);
            litShader.setVec3("uFogColor", fogColour);
            litShader.setFloat("uFogDensity", kFogDensity);
            renderer.draw(*simulation, terrain, litShader, alpha, frameDt,
                          selected);

            if (selected != 0) {
                const Organism* organism = simulation->find(selected);
                const Animal* animal =
                    dynamic_cast<const Animal*>(organism);
                if (animal != nullptr) {
                    const glm::vec2 position =
                        interpolatedPosition(*animal, alpha);
                    const float baseY =
                        terrain.heightAt(position.x, position.y);
                    const float lineY = baseY + 1.5f * bodyRadius;
                    if (sightRing) {
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
            waterShader.setFloat("uTime",
                                 static_cast<float>(window.time()));
            waterShader.setVec3("uFogColor", fogColour);
            waterShader.setFloat("uFogDensity", kFogDensity);
            waterMesh.draw();
            window.endBlendPass();
        };

        if (screenshotMode) {
            window.clear(fogColour.r, fogColour.g, fogColour.b);
            const auto sceneStart = std::chrono::steady_clock::now();
            renderScene(0, 1.f, 0.f, true);
            const auto sceneEnd = std::chrono::steady_clock::now();
            const float sceneMs =
                std::chrono::duration<float, std::milli>(sceneEnd - sceneStart)
                    .count();

            // Build the UI after the world so the one-frame HUD can show the
            // real draw-call count and scene time. The UI still renders last.
            window.pollEvents();
            ui.beginFrame();
            const HudData hud =
                makeHudData(*simulation, 0, clock.paused(),
                            renderer.drawCalls(), 0.f, sceneMs);
            UiActions uiActions;
            drawControlsPanel(clock, follow, selectedId != 0, showSightRing,
                              runConfig, *simulation, seed, uiActions);
            drawInspectorPanel(*simulation, selectedId, uiActions);
            drawStatisticsPanel(*simulation);
            drawLineagePanel(*simulation, selectedId, uiActions);
            drawGeneticsPanel(*simulation, selectedId);
            drawHud(hud);
            ui.render();
            glFinish();

            const int width = window.width();
            const int height = window.height();
            if (width <= 0 || height <= 0) {
                throw std::runtime_error(
                    "Screenshot framebuffer has invalid dimensions");
            }
            const std::size_t rowBytes =
                static_cast<std::size_t>(width) * 3;
            std::vector<std::uint8_t> pixels(
                rowBytes * static_cast<std::size_t>(height));
            glPixelStorei(GL_PACK_ALIGNMENT, 1);
            glReadBuffer(GL_BACK);
            glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE,
                         pixels.data());

            std::vector<std::uint8_t> topDown(pixels.size());
            for (int y = 0; y < height; ++y) {
                std::copy_n(
                    pixels.begin() + static_cast<std::ptrdiff_t>(
                                         static_cast<std::size_t>(height - 1 - y) *
                                         rowBytes),
                    rowBytes,
                    topDown.begin() +
                        static_cast<std::ptrdiff_t>(
                            static_cast<std::size_t>(y) * rowBytes));
            }

            if (!writePng(options.screenshotPath, width, height, topDown)) {
                throw std::runtime_error(
                    "Failed to write screenshot: " + options.screenshotPath);
            }

            std::set<std::uint32_t> distinctColours;
            for (std::size_t i = 0; i < topDown.size(); i += 3) {
                const std::uint32_t packed =
                    (static_cast<std::uint32_t>(topDown[i]) << 16) |
                    (static_cast<std::uint32_t>(topDown[i + 1]) << 8) |
                    static_cast<std::uint32_t>(topDown[i + 2]);
                distinctColours.insert(packed);
            }
            std::cout << "Screenshot written to " << options.screenshotPath
                      << " (" << distinctColours.size()
                      << " distinct colours)" << std::endl;
            return distinctColours.size() >= 50 ? 0 : 1;
        }

        std::cout << "EcoSim controls: Space pause/resume, ',' speed / 2, "
                     "'.' speed * 2, R restart, N next seed, F follow, "
                     "left-click select, left-drag orbit, right-drag pan, "
                     "wheel zoom, WASD move, Shift faster, ESC quit."
                  << std::endl;
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
                makeHudData(*simulation, fps, clock.paused(),
                            renderer.drawCalls(), simMsAverage,
                            renderMsAverage);
            UiActions uiActions;
            drawControlsPanel(clock, follow, selectedId != 0, showSightRing,
                              runConfig, *simulation, seed, uiActions);
            drawInspectorPanel(*simulation, selectedId, uiActions);
            drawStatisticsPanel(*simulation);
            drawLineagePanel(*simulation, selectedId, uiActions);
            drawGeneticsPanel(*simulation, selectedId);
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
            const auto simulationStart = std::chrono::steady_clock::now();
            for (int i = 0; i < steps; ++i) {
                simulation->step();
            }
            const auto simulationEnd = std::chrono::steady_clock::now();
            const float simMs =
                std::chrono::duration<float, std::milli>(
                    simulationEnd - simulationStart)
                    .count();
            // Exponential moving averages keep the per-frame HUD costs legible.
            simMsAverage += (simMs - simMsAverage) * kEmaFactor;
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

            const auto renderStart = std::chrono::steady_clock::now();
            window.clear(fogColour.r, fogColour.g, fogColour.b);
            renderScene(selectedId, alpha, frameDt, showSightRing);
            ui.render();
            const auto renderEnd = std::chrono::steady_clock::now();
            const float renderMs =
                std::chrono::duration<float, std::milli>(renderEnd - renderStart)
                    .count();
            renderMsAverage += (renderMs - renderMsAverage) * kEmaFactor;

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
