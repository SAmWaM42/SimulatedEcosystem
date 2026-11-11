// EcoSim static rendering showcase: a procedural terrain, translucent water, and
// five lit primitives viewed through an orbit camera.
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/vec3.hpp>

#include "render/Assets.h"
#include "render/Camera.h"
#include "render/Mesh.h"
#include "render/MeshFactory.h"
#include "render/Shader.h"
#include "render/Window.h"
#include "world/Terrain.h"

int main(int argc, char** argv) {
    try {
        setExecutablePath(argv[0]);

        std::uint32_t seed = 1;
        if (argc > 1) {
            seed = static_cast<std::uint32_t>(std::stoul(argv[1]));
        }

        TerrainConfig terrainConfig;
        terrainConfig.seed = seed;
        const Terrain terrain(terrainConfig);

        Window window(1280, 720, "EcoSim Viewer");
        window.enableDepthAndCull();

        Shader litShader(assetPath("shaders/lit.vert"),
                         assetPath("shaders/lit.frag"));
        Shader terrainShader(assetPath("shaders/terrain.vert"),
                             assetPath("shaders/terrain.frag"));
        Shader waterShader(assetPath("shaders/water.vert"),
                           assetPath("shaders/water.frag"));

        Mesh terrainMesh(makeTerrainMesh(terrain));
        Mesh cubeMesh(makeCube());
        Mesh sphereMesh(makeSphere());
        Mesh cylinderMesh(makeCylinder());
        Mesh coneMesh(makeCone());
        Mesh waterMesh(makePlane());

        Camera camera(terrain);
        const glm::vec3 lightDirection =
            glm::normalize(glm::vec3(-0.4f, -1.0f, -0.3f));

        std::cout << "EcoSim viewer controls: left-drag orbit, right-drag pan, "
                     "wheel zoom, WASD move, shift faster, ESC quit"
                  << std::endl;

        double lastTime = window.time();
        int frameCount = 0;
        double fpsElapsed = 0.0;

        while (!window.shouldClose()) {
            window.pollEvents();
            const double now = window.time();
            const float deltaTime = static_cast<float>(now - lastTime);
            lastTime = now;

            const InputState input = window.input();
            camera.update(input, deltaTime, terrain);

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

            const float centre = terrain.config().size * 0.5f;
            const glm::vec3 colors[5] = {
                {0.85f, 0.25f, 0.20f}, {0.25f, 0.70f, 0.35f},
                {0.90f, 0.75f, 0.20f}, {0.60f, 0.30f, 0.80f},
                {0.20f, 0.70f, 0.75f}};
            const Mesh* meshes[5] = {&cubeMesh, &sphereMesh, &sphereMesh,
                                     &cylinderMesh, &coneMesh};
            const glm::vec3 scales[5] = {
                {60.f, 60.f, 60.f}, {60.f, 60.f, 60.f},
                {120.f, 60.f, 60.f}, {60.f, 60.f, 60.f},
                {60.f, 60.f, 60.f}};

            litShader.use();
            litShader.setMat4("uView", view);
            litShader.setMat4("uProj", projection);
            litShader.setVec3("uLightDir", lightDirection);
            litShader.setVec3("uCameraPos", cameraPosition);

            for (int i = 0; i < 5; ++i) {
                const float x = centre + static_cast<float>(i - 2) * 150.f;
                const float z = centre;
                const float ground = terrain.heightAt(x, z);
                const glm::mat4 model =
                    glm::translate(glm::mat4(1.f),
                                   glm::vec3(x, ground + 30.f, z)) *
                    glm::scale(glm::mat4(1.f), scales[i]);
                litShader.setMat4("uModel", model);
                litShader.setMat3("uNormalMatrix",
                                  glm::inverseTranspose(glm::mat3(model)));
                litShader.setVec3("uColor", colors[i]);
                litShader.setVec3("uEmissive", glm::vec3(0.f));
                meshes[i]->draw();
            }

            // Draw water last with alpha blending and no depth writes so the
            // opaque scene beneath it remains visible.
            window.beginBlendPass();

            const glm::mat4 waterModel =
                glm::translate(glm::mat4(1.f),
                               glm::vec3(centre, terrain.waterLevel(), centre)) *
                glm::scale(glm::mat4(1.f),
                           glm::vec3(terrain.config().size, 1.f,
                                     terrain.config().size));
            waterShader.use();
            waterShader.setMat4("uModel", waterModel);
            waterShader.setMat4("uView", view);
            waterShader.setMat4("uProj", projection);
            waterShader.setMat3("uNormalMatrix",
                                glm::inverseTranspose(glm::mat3(waterModel)));
            waterShader.setVec3("uLightDir", lightDirection);
            waterShader.setVec3("uCameraPos", cameraPosition);
            waterMesh.draw();

            window.endBlendPass();

            window.swapBuffers();

            ++frameCount;
            fpsElapsed += deltaTime;
            if (fpsElapsed >= 0.5) {
                const int fps = static_cast<int>(
                    static_cast<double>(frameCount) / fpsElapsed);
                window.setTitle("EcoSim Viewer - " + std::to_string(fps) +
                                " FPS");
                frameCount = 0;
                fpsElapsed = 0.0;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "EcoSim viewer error: " << error.what() << '\n';
        return 1;
    }

    return 0;
}

