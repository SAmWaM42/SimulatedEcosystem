// Terrain generation and sampling: seeded gradient-noise fBm with ridges and
// domain warp, bilinear heights, central-difference normals, and a
// percentile-derived water line.
#include "world/Terrain.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <vector>

#include <glm/geometric.hpp>

#include "core/Rng.h"

namespace {
// Five octaves at base frequency 3.5, lacunarity 2, gain 0.5.
constexpr int kOctaves = 5;
constexpr float kBaseFrequency = 3.5f;
constexpr float kLacunarity = 2.f;
constexpr float kGain = 0.5f;
constexpr float kRidgeWeight = 0.3f;
constexpr float kWarpFrequency = 2.f;
constexpr float kWarpStrength = 0.15f;
constexpr float kInvSqrt2 = 0.70710678118f;

// Perlin's quintic fade: 6t^5 - 15t^4 + 10t^3.
float fade(float t) {
    return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
}

// One of eight unit gradient directions selected by a permutation hash.
float gradientDot(int hash, float dx, float dz) {
    switch (hash & 7) {
    case 0:
        return dx;
    case 1:
        return (dx + dz) * kInvSqrt2;
    case 2:
        return dz;
    case 3:
        return (-dx + dz) * kInvSqrt2;
    case 4:
        return -dx;
    case 5:
        return (-dx - dz) * kInvSqrt2;
    case 6:
        return -dz;
    default:
        return (dx - dz) * kInvSqrt2;
    }
}
}  // namespace

Terrain::Terrain(const TerrainConfig& terrainConfig) : cfg(terrainConfig) {
    cfg.resolution = std::max(1, cfg.resolution);
    const std::size_t vertexCount =
        static_cast<std::size_t>(cfg.resolution) * cfg.resolution;
    heights.resize(vertexCount);

    // Fisher-Yates shuffle of 0..255 with the simulation's seeded RNG.
    std::array<int, 256> table{};
    std::iota(table.begin(), table.end(), 0);
    Rng rng(cfg.seed);
    for (int i = 255; i > 0; --i) {
        const int j = rng.uniformInt(0, i);
        std::swap(table[static_cast<std::size_t>(i)],
                  table[static_cast<std::size_t>(j)]);
    }
    for (int i = 0; i < 512; ++i) {
        permutation[static_cast<std::size_t>(i)] =
            table[static_cast<std::size_t>(i & 255)];
    }

    const float denominator =
        (cfg.resolution > 1) ? static_cast<float>(cfg.resolution - 1) : 1.f;
    float minimum = 0.f;
    float maximum = 0.f;
    for (int iz = 0; iz < cfg.resolution; ++iz) {
        for (int ix = 0; ix < cfg.resolution; ++ix) {
            const float x =
                cfg.size * static_cast<float>(ix) / denominator;
            const float z =
                cfg.size * static_cast<float>(iz) / denominator;
            const float raw = generateRawHeight(x, z);
            heights[static_cast<std::size_t>(iz) * cfg.resolution + ix] = raw;
            if (iz == 0 && ix == 0) {
                minimum = raw;
                maximum = raw;
            } else {
                minimum = std::min(minimum, raw);
                maximum = std::max(maximum, raw);
            }
        }
    }

    // Stretch the generated field to [0, 1], then bias it toward flat
    // lowlands and sharper peaks before applying the requested height.
    const float range = maximum - minimum;
    if (range > 1e-6f) {
        for (float& height : heights) {
            const float normalized = (height - minimum) / range;
            height = std::pow(normalized, 1.4f) * cfg.maxHeight;
        }
    } else {
        std::fill(heights.begin(), heights.end(), 0.f);
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

float Terrain::gradientNoise(float x, float z) const {
    const int ix = static_cast<int>(std::floor(x));
    const int iz = static_cast<int>(std::floor(z));
    const float fx = x - static_cast<float>(ix);
    const float fz = z - static_cast<float>(iz);
    const int x0 = ix & 255;
    const int z0 = iz & 255;
    const int x1 = (ix + 1) & 255;
    const int z1 = (iz + 1) & 255;

    const auto dotAt = [&](int latticeX, int latticeZ, float dx,
                           float dz) {
        const int hash =
            permutation[static_cast<std::size_t>(
                permutation[static_cast<std::size_t>(latticeX)] +
                latticeZ)] &
            7;
        return gradientDot(hash, dx, dz);
    };

    const float n00 = dotAt(x0, z0, fx, fz);
    const float n10 = dotAt(x1, z0, fx - 1.f, fz);
    const float n01 = dotAt(x0, z1, fx, fz - 1.f);
    const float n11 = dotAt(x1, z1, fx - 1.f, fz - 1.f);

    const float u = fade(fx);
    const float v = fade(fz);
    const float lower = n00 + (n10 - n00) * u;
    const float upper = n01 + (n11 - n01) * u;
    return lower + (upper - lower) * v;
}

float Terrain::fbm(float x, float z) const {
    float frequency = 1.f;
    float amplitude = 1.f;
    float sum = 0.f;
    float amplitudeSum = 0.f;
    for (int octave = 0; octave < kOctaves; ++octave) {
        sum += amplitude * gradientNoise(x * frequency, z * frequency);
        amplitudeSum += amplitude;
        frequency *= kLacunarity;
        amplitude *= kGain;
    }
    return sum / amplitudeSum;
}

float Terrain::ridged(float x, float z) const {
    float frequency = 1.f;
    float amplitude = 1.f;
    float sum = 0.f;
    float amplitudeSum = 0.f;
    for (int octave = 0; octave < kOctaves; ++octave) {
        const float noise = gradientNoise(x * frequency, z * frequency);
        sum += amplitude * (1.f - std::fabs(noise));
        amplitudeSum += amplitude;
        frequency *= kLacunarity;
        amplitude *= kGain;
    }
    return sum / amplitudeSum;
}

float Terrain::generateRawHeight(float x, float z) const {
    const float u = (cfg.size > 0.f) ? x / cfg.size : 0.f;
    const float v = (cfg.size > 0.f) ? z / cfg.size : 0.f;

    // A low-frequency warp offsets both lookup axes by about 0.15 of a lattice
    // cell, breaking up the grid-aligned look without displacing the terrain
    // globally.
    const float warpX =
        gradientNoise(u * kWarpFrequency + 19.1f, v * kWarpFrequency - 7.7f);
    const float warpZ =
        gradientNoise(u * kWarpFrequency - 11.3f, v * kWarpFrequency + 5.9f);
    const float sampleX =
        u * kBaseFrequency + warpX * kWarpStrength;
    const float sampleZ =
        v * kBaseFrequency + warpZ * kWarpStrength;

    const float fbmHeight = (fbm(sampleX, sampleZ) + 1.f) * 0.5f;
    const float ridgeHeight = ridged(sampleX, sampleZ);
    return (1.f - kRidgeWeight) * fbmHeight +
           kRidgeWeight * ridgeHeight;
}
