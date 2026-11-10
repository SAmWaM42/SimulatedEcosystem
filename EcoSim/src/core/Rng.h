// Thin wrapper around std::mt19937 so every part of EcoSim draws from one
// seeded stream; keeping it here avoids duplicating seeding logic.
#pragma once

#include <cstdint>
#include <random>

// Single seeded random-number generator shared by simulation and genetics.
class Rng {
public:
    // Creates a generator with a fixed seed so runs and tests are reproducible.
    explicit Rng(uint32_t seed) : gen(seed) {}

    // Returns a real value in [lo, hi); used for continuous traits and jitter.
    float uniform(float lo, float hi) {
        return std::uniform_real_distribution<float>(lo, hi)(gen);
    }

    // Returns an integer in [lo, hi] with both ends inclusive; used for indices.
    int uniformInt(int lo, int hi) {
        return std::uniform_int_distribution<int>(lo, hi)(gen);
    }

    // Returns true with probability p; used for mutation and other coin flips.
    bool chance(float p) {
        return uniform(0.0f, 1.0f) < p;
    }

    // Returns a Gaussian value; used for mutation step sizes.
    float normal(float mean, float sd) {
        return std::normal_distribution<float>(mean, sd)(gen);
    }

    // Exposes the engine for standard distributions such as std::discrete_distribution.
    std::mt19937& engine() {
        return gen;
    }

private:
    std::mt19937 gen;
};
