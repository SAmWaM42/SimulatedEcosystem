// Headless driver for the simulation: prints population and sight statistics
// over time so the foundation can be checked without any graphics.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>

#include "sim/Simulation.h"

namespace {
// Prints one fixed-width row of population statistics.
void printRow(const Simulation& sim) {
    std::printf("%8.1f %8d %10d %10d %14.2f %14.2f\n",
                sim.time(), sim.count(Kind::Plant),
                sim.count(Kind::Herbivore), sim.count(Kind::Predator),
                sim.meanSight(Kind::Herbivore), sim.meanSight(Kind::Predator));
}
}  // namespace

// Runs the requested number of simulated seconds and prints periodic snapshots.
int main(int argc, char** argv) {
    std::uint32_t seed = 1;
    float seconds = 600.f;
    if (argc > 1) {
        seed = static_cast<std::uint32_t>(std::stoul(argv[1]));
    }
    if (argc > 2) {
        seconds = std::stof(argv[2]);
    }

    const SimConfig cfg;
    Simulation sim(cfg, seed);
    sim.populate();

    const int stepsPerPrint =
        std::max(1, static_cast<int>(30.f / cfg.fixedDt + 0.5f));
    int totalSteps = static_cast<int>(seconds / cfg.fixedDt + 0.5f);
    if (totalSteps < 0) {
        totalSteps = 0;
    }

    std::printf("%8s %8s %10s %10s %14s %14s\n", "time", "plants",
                "herbivores", "predators", "herbSight", "predSight");
    printRow(sim);

    for (int step = 1; step <= totalSteps; ++step) {
        sim.step();
        if (step % stepsPerPrint == 0 || step == totalSteps) {
            printRow(sim);
        }
    }

    return 0;
}
