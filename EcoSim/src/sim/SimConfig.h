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
