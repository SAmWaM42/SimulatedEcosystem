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
    int plantMaxNeighbours = 3;
    float plantEnergyValue = 30.f;
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
    float eatReach = 16.f;             // distance at which an animal can eat / attack
    float preyEnergyValue = 60.f;      // energy a predator gains from a herbivore
    float predatorCatchChance = 0.6f;  // chance a reached attack succeeds
    float predatorMissCooldown = 4.f;  // seconds a predator gives up after a miss
    // herbivores
    int initialHerbivores = 30;
    float herbivoreSpeed = 40.f;
    float herbivoreMaxAge = 300.f;
    // predators
    int initialPredators = 8;
    float predatorSpeed = 48.f;
    float predatorMaxAge = 400.f;
    // genetics
    genetics::GeneticsConfig genetics;
};
