// Reproduction implementation: eligibility checks, litter creation, and the
// fixed-at-conception genomes carried by the mother until birth.
#include "sim/Reproduction.h"

#include <algorithm>

#include "sim/Organism.h"
#include "sim/Simulation.h"

bool conceive(Animal& mother, Animal& father, Simulation& sim) {
    const SimConfig& cfg = sim.config();
    if (mother.kind() != father.kind() ||
        mother.sex != genetics::Sex::Female ||
        father.sex != genetics::Sex::Male ||
        !mother.canMate(cfg) || !father.canMate(cfg)) {
        return false;
    }

    const Kind kind = mother.kind();
    int cap = 0;
    if (kind == Kind::Herbivore) {
        cap = cfg.maxHerbivores;
    } else if (kind == Kind::Predator) {
        cap = cfg.maxPredators;
    } else {
        return false;
    }
    if (sim.count(kind) >= cap) {
        return false;
    }

    const int litter = sim.rng().uniformInt(cfg.litterMin, cfg.litterMax);
    for (int i = 0; i < litter; ++i) {
        mother.embryos.push_back(
            genetics::cross(mother.genome, father.genome, cfg.genetics,
                            sim.rng()));
    }

    mother.gestationTimer = cfg.gestationTime;
    mother.embryoFatherId = father.id;
    mother.embryoGeneration =
        std::max(mother.generation, father.generation) + 1;
    mother.energy -= cfg.pregnancyEnergyCost;
    father.energy -= cfg.matingEnergyCost;
    father.reproCooldown = cfg.maleCooldown;
    return true;
}
