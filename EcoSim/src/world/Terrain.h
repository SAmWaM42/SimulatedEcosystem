// Procedural height field for the EcoSim world; GL-free so it can be tested
// headless and reused by the renderer.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include <glm/vec3.hpp>

// Tunables for the deterministic gradient-noise terrain.
struct TerrainConfig {
    int resolution = 257;      // vertices per side
    float size = 3000.f;       // world units per side
    float maxHeight = 120.f;
    float waterFraction = 0.2f;  // share of the map under water
    std::uint32_t seed = 20;
};

// Deterministic height field with bilinear sampling and a percentile water line.
class Terrain {
public:
    // Generates the warped fBm/ridged height field and derives the water level.
    explicit Terrain(const TerrainConfig& terrainConfig);
    // Bilinear height between grid points; points outside the map are clamped.
    float heightAt(float x, float z) const;
    // Unit surface normal from central differences of heightAt.
    glm::vec3 normalAt(float x, float z) const;
    // Height below which the map is considered water.
    float waterLevel() const;
    // True when the sampled height is below the water level.
    bool isWater(float x, float z) const;
    // Returns the configuration used to generate this terrain.
    const TerrainConfig& config() const;
    // Number of vertices per side.
    int resolution() const;
    // Height at one generated grid vertex; indices are clamped.
    float vertexHeight(int ix, int iz) const;

private:
    // Seeded 2D gradient (Perlin) noise in roughly [-1, 1].
    float gradientNoise(float x, float z) const;
    // Five-octave fBm normalised to roughly [-1, 1].
    float fbm(float x, float z) const;
    // Five-octave ridged noise normalised to [0, 1].
    float ridged(float x, float z) const;
    // Warped blend of fBm and ridged noise before global rescaling.
    float generateRawHeight(float x, float z) const;

    TerrainConfig cfg;
    std::array<int, 512> permutation{};
    std::vector<float> heights;  // row-major, iz * resolution + ix
    float waterLevelValue = 0.f;
};
