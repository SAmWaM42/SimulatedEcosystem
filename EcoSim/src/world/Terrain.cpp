// Terrain generation and sampling: seeded value-noise fBm, bilinear heights,
// central-difference normals, and a percentile-derived water line.
#include "world/Terrain.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

#include <glm/geometric.hpp>

namespace {
// Four octaves, frequency 4 at octave 0, lacunarity 2, gain 0.5.
constexpr int kOctaves = 4;
constexpr float kBaseFrequency = 4.f;
constexpr float kLacunarity = 2.f;
constexpr float kGain = 0.5f;
}  // namespace

Terrain::Terrain(const TerrainConfig& terrainConfig) : cfg(terrainConfig) {
    cfg.resolution = std::max(1, cfg.resolution);
    const std::size_t vertexCount =
        static_cast<std::size_t>(cfg.resolution) * cfg.resolution;
    heights.resize(vertexCount);

    const float denominator =
        (cfg.resolution > 1) ? static_cast<float>(cfg.resolution - 1) : 1.f;
    for (int iz = 0; iz < cfg.resolution; ++iz) {
        for (int ix = 0; ix < cfg.resolution; ++ix) {
            const float x =
                cfg.size * static_cast<float>(ix) / denominator;
            const float z =
                cfg.size * static_cast<float>(iz) / denominator;
            heights[static_cast<std::size_t>(iz) * cfg.resolution + ix] =
                generateHeight(x, z);
        }
    }

    // The water level is the height at the requested percentile, so the
    // fraction of submerged vertices is stable for every seed.
    std::vector<float> sorted = heights;
    std::sort(sorted.begin(), sorted.end());
    if (!sorted.empty()) {
        const std::size_t index = std::clamp(
            static_cast<std::size_t>(cfg.waterFraction *
                                     static_cast<float>(sorted.size() - 1)),
            std::size_t{0}, sorted.size() - 1);
        waterLevelValue = sorted[index];
    }
}

float Terrain::heightAt(float x, float z) const {
    if (cfg.resolution <= 1 || heights.empty()) {
        return heights.empty() ? 0.f : heights[0];
    }

    const float clampedX = std::clamp(x, 0.f, cfg.size);
    const float clampedZ = std::clamp(z, 0.f, cfg.size);
    const float gridX = clampedX / cfg.size *
                        static_cast<float>(cfg.resolution - 1);
    const float gridZ = clampedZ / cfg.size *
                        static_cast<float>(cfg.resolution - 1);
    int ix = static_cast<int>(std::floor(gridX));
    int iz = static_cast<int>(std::floor(gridZ));
    ix = std::clamp(ix, 0, cfg.resolution - 2);
    iz = std::clamp(iz, 0, cfg.resolution - 2);
    const float fx = gridX - static_cast<float>(ix);
    const float fz = gridZ - static_cast<float>(iz);

    const float h00 = vertexHeight(ix, iz);
    const float h10 = vertexHeight(ix + 1, iz);
    const float h01 = vertexHeight(ix, iz + 1);
    const float h11 = vertexHeight(ix + 1, iz + 1);
    return (h00 * (1.f - fx) + h10 * fx) * (1.f - fz) +
           (h01 * (1.f - fx) + h11 * fx) * fz;
}

glm::vec3 Terrain::normalAt(float x, float z) const {
    const float denominator =
        (cfg.resolution > 1) ? static_cast<float>(cfg.resolution - 1) : 1.f;
    const float step = cfg.size / denominator;
    const float heightLeft = heightAt(x - step, z);
    const float heightRight = heightAt(x + step, z);
    const float heightDown = heightAt(x, z - step);
    const float heightUp = heightAt(x, z + step);
    return glm::normalize(glm::vec3(heightLeft - heightRight, 2.f * step,
                                    heightDown - heightUp));
}

float Terrain::waterLevel() const {
    return waterLevelValue;
}

bool Terrain::isWater(float x, float z) const {
    return heightAt(x, z) < waterLevelValue;
}

const TerrainConfig& Terrain::config() const {
    return cfg;
}

int Terrain::resolution() const {
    return cfg.resolution;
}

float Terrain::vertexHeight(int ix, int iz) const {
    if (heights.empty()) {
        return 0.f;
    }
    ix = std::clamp(ix, 0, cfg.resolution - 1);
    iz = std::clamp(iz, 0, cfg.resolution - 1);
    return heights[static_cast<std::size_t>(iz) * cfg.resolution + ix];
}

float Terrain::hash01(int x, int z) const {
    std::uint32_t h = static_cast<std::uint32_t>(x) * 0x8da6b343u;
    h ^= static_cast<std::uint32_t>(z) * 0xd8163841u;
    h ^= cfg.seed * 0xcb1ab31fu;
    h ^= h >> 13;
    h *= 0x85ebca6bu;
    h ^= h >> 16;
    return static_cast<float>(h & 0x00ffffffu) / 16777216.f;
}

float Terrain::valueNoise(float x, float z) const {
    const int ix = static_cast<int>(std::floor(x));
    const int iz = static_cast<int>(std::floor(z));
    const float fx = x - static_cast<float>(ix);
    const float fz = z - static_cast<float>(iz);
    const float sx = fx * fx * (3.f - 2.f * fx);
    const float sz = fz * fz * (3.f - 2.f * fz);

    const float v00 = hash01(ix, iz);
    const float v10 = hash01(ix + 1, iz);
    const float v01 = hash01(ix, iz + 1);
    const float v11 = hash01(ix + 1, iz + 1);
    const float lower = v00 + (v10 - v00) * sx;
    const float upper = v01 + (v11 - v01) * sx;
    return lower + (upper - lower) * sz;
}

float Terrain::generateHeight(float x, float z) const {
    const float u = (cfg.size > 0.f) ? x / cfg.size : 0.f;
    const float v = (cfg.size > 0.f) ? z / cfg.size : 0.f;

    float frequency = kBaseFrequency;
    float amplitude = 1.f;
    float sum = 0.f;
    float amplitudeSum = 0.f;
    for (int octave = 0; octave < kOctaves; ++octave) {
        sum += amplitude * valueNoise(u * frequency, v * frequency);
        amplitudeSum += amplitude;
        frequency *= kLacunarity;
        amplitude *= kGain;
    }
    return (sum / amplitudeSum) * cfg.maxHeight;
}
