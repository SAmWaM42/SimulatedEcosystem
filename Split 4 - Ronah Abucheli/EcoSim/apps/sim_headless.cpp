// Headless driver for the simulation: prints population, sight, reproduction,
// rescue, and generation statistics and optionally exports the stats CSV.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

#include "sim/Simulation.h"

namespace {
// Returns the highest generation among currently living animals.
int maxLivingGeneration(const Simulation& sim) {
    int maximum = 0;
    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        const Animal* animal = dynamic_cast<const Animal*>(organism.get());
        if (animal != nullptr && animal->alive) {
            maximum = std::max(maximum, animal->generation);
        }
    }
    return maximum;
}

// Prints one fixed-width row of population and reproduction statistics.
void printRow(const Simulation& sim) {
    std::printf("%8.1f %8d %10d %10d %14.2f %14.2f %10d %11d %12d\n",
                sim.time(), sim.count(Kind::Plant),
                sim.count(Kind::Herbivore), sim.count(Kind::Predator),
                sim.meanSight(Kind::Herbivore), sim.meanSight(Kind::Predator),
                sim.birthCount(), sim.immigrantCount(),
                maxLivingGeneration(sim));
}
}  // namespace

// Runs the requested number of simulated seconds and prints periodic snapshots.
int main(int argc, char** argv) {
    std::uint32_t seed = 1;
    float seconds = 600.f;
    std::string csvPath;
    if (argc > 1) {
        seed = static_cast<std::uint32_t>(std::stoul(argv[1]));
    }
    if (argc > 2) {
        seconds = std::stof(argv[2]);
    }
    if (argc > 3) {
        csvPath = argv[3];
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

    std::printf("%8s %8s %10s %10s %14s %14s %10s %11s %12s\n", "time",
                "plants", "herbivores", "predators", "herbSight", "predSight",
                "births", "immigrants", "maxGen");
    printRow(sim);

    for (int step = 1; step <= totalSteps; ++step) {
        sim.step();
        if (step % stepsPerPrint == 0 || step == totalSteps) {
            printRow(sim);
        }
    }

    if (!csvPath.empty() && !sim.stats().writeCsv(csvPath)) {
        std::fprintf(stderr, "Failed to write CSV: %s\n", csvPath.c_str());
        return 1;
    }

    return 0;
}
