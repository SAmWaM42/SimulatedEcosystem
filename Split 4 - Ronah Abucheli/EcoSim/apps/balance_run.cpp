// Balance harness: runs the default simulation for several seeds and reports
// population minima/finals, sight change, reproduction, and extinction status.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>

#include "sim/Simulation.h"

namespace {
// Converts a simulated duration to the nearest whole number of fixed steps.
int stepsForSeconds(const SimConfig& cfg, float seconds) {
    return static_cast<int>(seconds / cfg.fixedDt + 0.5f);
}
}  // namespace

// Runs one simulation per seed and prints one comparison row per seed.
int main(int argc, char** argv) {
    int seeds = 5;
    float seconds = 1200.f;
    bool rescue = true;
    if (argc > 1) {
        seeds = std::stoi(argv[1]);
    }
    if (argc > 2) {
        seconds = std::stof(argv[2]);
    }
    if (argc > 3) {
        rescue = std::stoi(argv[3]) != 0;
    }

    std::printf("%4s %8s %9s %8s %9s %12s %15s %13s %8s %11s %s\n",
                "seed", "herbMin", "herbFinal", "predMin", "predFinal",
                "plants", "herbSightStart", "herbSightEnd", "births",
                "immigrants", "status");

    for (int seed = 1; seed <= seeds; ++seed) {
        SimConfig cfg;
        cfg.enableRescue = rescue;
        Simulation sim(cfg, static_cast<std::uint32_t>(seed));
        sim.populate();

        const float startSight = sim.meanSight(Kind::Herbivore);
        int minHerbivores = sim.count(Kind::Herbivore);
        int minPredators = sim.count(Kind::Predator);

        const int steps = stepsForSeconds(cfg, seconds);
        for (int i = 0; i < steps; ++i) {
            sim.step();
            minHerbivores = std::min(minHerbivores,
                                     sim.count(Kind::Herbivore));
            minPredators = std::min(minPredators,
                                    sim.count(Kind::Predator));
        }

        const int finalHerbivores = sim.count(Kind::Herbivore);
        const int finalPredators = sim.count(Kind::Predator);
        const bool ok = finalHerbivores > 0 && finalPredators > 0;

        std::printf("%4d %8d %9d %8d %9d %12d %15.2f %13.2f %8d %11d %s\n",
                    seed, minHerbivores, finalHerbivores, minPredators,
                    finalPredators, sim.count(Kind::Plant), startSight,
                    sim.meanSight(Kind::Herbivore), sim.birthCount(),
                    sim.immigrantCount(), ok ? "OK" : "EXTINCT");
    }

    return 0;
}
