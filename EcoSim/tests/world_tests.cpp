// Headless property tests for Split 5: terrain determinism/sampling, water,
// normals, mesh winding, and orbit-camera constraints.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include "core/Rng.h"
#include "render/Camera.h"
#include "render/Input.h"
#include "render/MeshData.h"
#include "render/MeshFactory.h"
#include "world/Terrain.h"

namespace {
// Checks index ranges, unit normals, non-degeneracy, and outward winding.
void checkMesh(const MeshData& mesh) {
    REQUIRE(!mesh.vertices.empty());
    REQUIRE(!mesh.indices.empty());
    REQUIRE(mesh.indices.size() % 3 == 0);

    for (std::uint32_t index : mesh.indices) {
        REQUIRE(index < mesh.vertices.size());
    }
    for (const Vertex& vertex : mesh.vertices) {
        CHECK(glm::length(vertex.normal) ==
              doctest::Approx(1.f).epsilon(1e-3));
    }

    for (std::size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Vertex& v0 = mesh.vertices[mesh.indices[i]];
        const Vertex& v1 = mesh.vertices[mesh.indices[i + 1]];
        const Vertex& v2 = mesh.vertices[mesh.indices[i + 2]];
        const glm::vec3 geometricNormal =
            glm::cross(v1.position - v0.position, v2.position - v0.position);
        const glm::vec3 averageNormal =
            glm::normalize(v0.normal + v1.normal + v2.normal);
        CHECK(glm::length(geometricNormal) > 1e-8f);
        CHECK(glm::dot(geometricNormal, averageNormal) > 0.f);
    }
}
}  // namespace

TEST_CASE("terrain is deterministic for a seed") {
    TerrainConfig cfg;
    cfg.resolution = 33;
    cfg.size = 1000.f;
    cfg.maxHeight = 100.f;
    cfg.seed = 7;

    const Terrain first(cfg);
    const Terrain second(cfg);
    for (int iz = 0; iz < cfg.resolution; ++iz) {
        for (int ix = 0; ix < cfg.resolution; ++ix) {
            CHECK(first.vertexHeight(ix, iz) ==
                  second.vertexHeight(ix, iz));
        }
    }

    TerrainConfig otherCfg = cfg;
    otherCfg.seed = 8;
    const Terrain other(otherCfg);
    bool differs = false;
    for (int iz = 0; iz < cfg.resolution; ++iz) {
        for (int ix = 0; ix < cfg.resolution; ++ix) {
            if (first.vertexHeight(ix, iz) != other.vertexHeight(ix, iz)) {
                differs = true;
            }
        }
    }
    CHECK(differs);
}

TEST_CASE("terrain sampling matches vertices and stays bounded") {
    TerrainConfig cfg;
    cfg.resolution = 33;
    cfg.size = 1000.f;
    cfg.maxHeight = 100.f;
    cfg.seed = 11;
    const Terrain terrain(cfg);

    const int last = cfg.resolution - 1;
    for (int iz = 0; iz < cfg.resolution; ++iz) {
        for (int ix = 0; ix < cfg.resolution; ++ix) {
            const float x = cfg.size * static_cast<float>(ix) /
                            static_cast<float>(last);
            const float z = cfg.size * static_cast<float>(iz) /
                            static_cast<float>(last);
            CHECK(terrain.heightAt(x, z) ==
                  doctest::Approx(terrain.vertexHeight(ix, iz))
                      .epsilon(1e-5));

            const float height = terrain.vertexHeight(ix, iz);
            CHECK(height >= 0.f);
            CHECK(height <= cfg.maxHeight);
        }
    }

    Rng rng(12);
    for (int sample = 0; sample < 200; ++sample) {
        const float x = rng.uniform(0.f, cfg.size);
        const float z = rng.uniform(0.f, cfg.size);
        int ix = static_cast<int>(x / cfg.size * static_cast<float>(last));
        int iz = static_cast<int>(z / cfg.size * static_cast<float>(last));
        ix = std::clamp(ix, 0, last - 1);
        iz = std::clamp(iz, 0, last - 1);

        const float h00 = terrain.vertexHeight(ix, iz);
        const float h10 = terrain.vertexHeight(ix + 1, iz);
        const float h01 = terrain.vertexHeight(ix, iz + 1);
        const float h11 = terrain.vertexHeight(ix + 1, iz + 1);
        const float low = std::min(std::min(h00, h10), std::min(h01, h11));
        const float high = std::max(std::max(h00, h10), std::max(h01, h11));
        const float height = terrain.heightAt(x, z);
        CHECK(height >= low - 1e-4f);
        CHECK(height <= high + 1e-4f);
    }
}

TEST_CASE("water covers roughly the requested fraction") {
    TerrainConfig cfg;
    cfg.resolution = 65;
    cfg.size = 1200.f;
    cfg.maxHeight = 90.f;
    cfg.waterFraction = 0.2f;
    cfg.seed = 13;
    const Terrain terrain(cfg);

    int submerged = 0;
    for (int iz = 0; iz < cfg.resolution; ++iz) {
        for (int ix = 0; ix < cfg.resolution; ++ix) {
            if (terrain.vertexHeight(ix, iz) < terrain.waterLevel()) {
                ++submerged;
            }
        }
    }
    const float fraction =
        static_cast<float>(submerged) /
        static_cast<float>(cfg.resolution * cfg.resolution);
    CHECK(fraction >= 0.17f);
    CHECK(fraction <= 0.23f);

    Rng rng(14);
    for (int sample = 0; sample < 200; ++sample) {
        const float x = rng.uniform(0.f, cfg.size);
        const float z = rng.uniform(0.f, cfg.size);
        CHECK(terrain.isWater(x, z) ==
              (terrain.heightAt(x, z) < terrain.waterLevel()));
    }
}

TEST_CASE("terrain normals are unit and point upward") {
    TerrainConfig cfg;
    cfg.resolution = 65;
    cfg.size = 1200.f;
    cfg.maxHeight = 90.f;
    cfg.seed = 15;
    const Terrain terrain(cfg);

    Rng rng(16);
    for (int sample = 0; sample < 200; ++sample) {
        const float x = rng.uniform(0.f, cfg.size);
        const float z = rng.uniform(0.f, cfg.size);
        const glm::vec3 normal = terrain.normalAt(x, z);
        CHECK(glm::length(normal) == doctest::Approx(1.f).epsilon(1e-4));
        CHECK(normal.y > 0.f);
    }
}

TEST_CASE("mesh factory produces valid outward-facing meshes") {
    checkMesh(makeCube());
    checkMesh(makeSphere());
    checkMesh(makeCylinder());
    checkMesh(makeCone());
    checkMesh(makePlane());

    TerrainConfig cfg;
    cfg.resolution = 9;
    cfg.size = 200.f;
    cfg.maxHeight = 30.f;
    cfg.seed = 17;
    const Terrain terrain(cfg);
    checkMesh(makeTerrainMesh(terrain));

    const MeshData sphere = makeSphere();
    for (const Vertex& vertex : sphere.vertices) {
        CHECK(glm::length(vertex.position) ==
              doctest::Approx(0.5f).epsilon(1e-3));
    }
}

TEST_CASE("default terrain is varied and has no adjacent spikes") {
    TerrainConfig cfg;
    const Terrain terrain(cfg);

    float minimum = terrain.vertexHeight(0, 0);
    float maximum = minimum;
    float maximumStep = 0.f;
    for (int iz = 0; iz < cfg.resolution; ++iz) {
        for (int ix = 0; ix < cfg.resolution; ++ix) {
            const float height = terrain.vertexHeight(ix, iz);
            minimum = std::min(minimum, height);
            maximum = std::max(maximum, height);
            if (ix > 0) {
                maximumStep = std::max(
                    maximumStep,
                    std::fabs(height - terrain.vertexHeight(ix - 1, iz)));
            }
            if (iz > 0) {
                maximumStep = std::max(
                    maximumStep,
                    std::fabs(height - terrain.vertexHeight(ix, iz - 1)));
            }
        }
    }

    INFO("max step ", maximumStep, " of ", cfg.maxHeight);
    CHECK(maximum - minimum > 0.5f * cfg.maxHeight);
    CHECK(maximumStep <= 0.12f * cfg.maxHeight);
}

TEST_CASE("orbit camera clamps distance, pitch, and target") {
    TerrainConfig cfg;
    cfg.resolution = 65;
    cfg.size = 1000.f;
    cfg.maxHeight = 100.f;
    cfg.seed = 21;
    const Terrain terrain(cfg);

    Camera defaultCamera(terrain);
    CHECK(defaultCamera.eye().y > defaultCamera.target().y);

    Camera scrollCamera(terrain);
    InputState scrollIn;
    scrollIn.scroll = 60.f;
    scrollCamera.update(scrollIn, 0.016f, terrain);
    CHECK(scrollCamera.distance() >= 30.f);
    scrollIn.scroll = -60.f;
    scrollCamera.update(scrollIn, 0.016f, terrain);
    CHECK(scrollCamera.distance() <= 4500.f);

    Camera pitchCamera(terrain);
    InputState pitchIn;
    pitchIn.orbitDrag = true;
    pitchIn.mouseDelta = glm::vec2(0.f, 10000.f);
    pitchCamera.update(pitchIn, 0.016f, terrain);
    CHECK(pitchCamera.pitch() <= 1.52f);
    pitchIn.mouseDelta = glm::vec2(0.f, -10000.f);
    pitchCamera.update(pitchIn, 0.016f, terrain);
    CHECK(pitchCamera.pitch() >= 0.09f);

    Camera eyeCamera(terrain);
    InputState eyeIn;
    eyeIn.scroll = 60.f;
    eyeIn.orbitDrag = true;
    eyeIn.mouseDelta = glm::vec2(0.f, -10000.f);
    eyeCamera.update(eyeIn, 0.016f, terrain);
    const glm::vec3 eyePosition = eyeCamera.eye();
    CHECK(eyePosition.y >=
          terrain.heightAt(eyePosition.x, eyePosition.z) + 8.f - 1e-3f);

    Camera moveCamera(terrain);
    InputState moveIn;
    moveIn.moveForward = true;
    moveIn.fast = true;
    for (int step = 0; step < 100; ++step) {
        moveCamera.update(moveIn, 0.05f, terrain);
    }
    CHECK(moveCamera.target().x >= 0.f);
    CHECK(moveCamera.target().x <= terrain.config().size);
    CHECK(moveCamera.target().z >= 0.f);
    CHECK(moveCamera.target().z <= terrain.config().size);
}
