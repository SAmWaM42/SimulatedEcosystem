// Plain configuration values for the headless simulation; defaults are the
// single source of truth so no file loading is needed in this split.
#pragma once

#include "genetics/Genetics.h"

// All tunable world, plant, animal, and genetics settings in one plain struct.
struct SimConfig {
    // world
    float worldSize = 3000.f;
    float fixedDt = 1.f / 30.f;
    float boundaryMargin = 150.f;
    float spatialCellSize = 200.f;
    // plants
    int initialPlants = 300;
    int maxPlants = 800;
    float plantSpawnMin = 8.f;
    float plantSpawnMax = 20.f;
    float plantSpreadRadius = 150.f;
    float plantCrowdRadius = 60.f;
    int plantMaxNeighbours = 2;
    float plantEnergyValue = 80.f;
    // animals (shared)
    float maxEnergy = 100.f;
    float startEnergy = 80.f;
    float baseUpkeep = 0.5f;
    float sightUpkeep = 0.004f;
    float bodyRadius = 8.f;
    float wanderTurnMin = 1.5f;
    float wanderTurnMax = 4.0f;
    float wanderTurnAngle = 0.8f;
    float senseInterval = 0.3f;        // seconds between perception scans
    float hungerThreshold = 0.8f;      // animals look for food below this fraction of maxEnergy
    float eatReach = 32.f;             // distance at which an animal can eat / attack
    float preyEnergyValue = 75.f;      // energy a predator gains from a herbivore
    float predatorCatchChance = 0.7f; // chance a reached attack succeeds
    float predatorMissCooldown = 4.f;  // seconds a predator gives up after a miss
    // reproduction
    float maturityAge = 15.f;           // seconds before an animal can mate
    float mateEnergyThreshold = 0.6f;  // fraction of maxEnergy needed to be ready to mate
    float mateReach = 16.f;
    float gestationTime = 12.f;
    int   litterMin = 1;
    int   litterMax = 2;
    float pregnancyEnergyCost = 10.f;   // paid by the mother at conception
    float matingEnergyCost = 4.f;       // paid by the father
    float postBirthCooldown = 20.f;     // mother cannot conceive again for this long
    float maleCooldown = 10.f;          // father cannot mate again for this long
    float offspringStartEnergy = 80.f;
    float birthScatter = 20.f;          // offspring appear within this distance of the mother
    int   maxHerbivores = 200;
    int   maxPredators = 60;            // hard population caps
    // population rescue (demo safety net)
    bool  enableRescue = true;
    int   rescueFloor = 6;              // per animal kind
    float rescueInterval = 30.f;        // seconds
    // stats
    float statsInterval = 5.f;
    // herbivores
    int initialHerbivores = 30;
    float herbivoreSpeed = 40.f;
    float herbivoreMaxAge = 300.f;
    // predators
    int initialPredators = 10;
    float predatorSpeed = 50.f;
    float predatorMaxAge = 400.f;
    float predatorUpkeepMultiplier = 0.2f;  // scales predator maintenance energy
    float predatorStartEnergy = 100.f;  // founders/immigrants start full
    float predatorSightMultiplier = 2.0f;  // effective hunting sense multiplier
    int predatorPreyRefuge = 20;           // stop hunting when herbivores are this scarce
    // genetics
    genetics::GeneticsConfig genetics;
};

// Copies only the settings that the UI may change while a simulation is
// running. Other fields are applied when the simulation is recreated.
inline void copyLiveTunables(const SimConfig& from, SimConfig& to) {
    to.genetics.sightMutationRate = from.genetics.sightMutationRate;
    to.genetics.sightMutationSd = from.genetics.sightMutationSd;
    to.genetics.colourMutationRate = from.genetics.colourMutationRate;
    to.sightUpkeep = from.sightUpkeep;
    to.plantSpawnMin = from.plantSpawnMin;
    to.plantSpawnMax = from.plantSpawnMax;
    to.plantEnergyValue = from.plantEnergyValue;
    to.predatorCatchChance = from.predatorCatchChance;
    to.enableRescue = from.enableRescue;
}
