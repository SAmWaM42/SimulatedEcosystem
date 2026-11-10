// Simulation manager: owns the population, the shared RNG, the world, and the
// spatial grid, and advances all organisms one fixed timestep at a time.
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "core/Rng.h"
#include "sim/Organism.h"
#include "sim/SimConfig.h"
#include "sim/SpatialGrid.h"
#include "sim/World.h"

// Owns every organism and coordinates population updates, births, and death.
class Simulation {
public:
    // Builds a simulation with the given settings and deterministic RNG seed.
    Simulation(const SimConfig& cfg, uint32_t seed);

    // Creates the configured founder plants and animals at random positions.
    void populate();

    // Advances the whole simulation by exactly cfg.fixedDt seconds.
    void step();

    // Queues an organism; it joins the main population at the end of the step.
    void addOrganism(std::unique_ptr<Organism> o);

    // Returns the next unique organism id, starting at 1.
    uint32_t nextId();

    // Returns the immutable settings shared by every organism.
    const SimConfig& config() const;
    // Returns the shared RNG so behaviour and tests draw from one stream.
    Rng& rng();
    // Returns the world used for containment and clamping.
    const World& world() const;
    // Returns the grid rebuilt at the start of each step for proximity queries.
    SpatialGrid& grid();
    // Returns the simulated time in seconds.
    double time() const;
    // Returns the current population, excluding organisms still queued.
    const std::vector<std::unique_ptr<Organism>>& organisms() const;

    // Living organisms of a kind, including additions queued this step so the
    // plant cap sees every birth before it is inserted into the grid.
    int count(Kind k) const;

    // Mean cached sight for an animal kind, including queued additions, or 0
    // when none of that kind live.
    float meanSight(Kind k) const;

    // Moves all queued organisms into the main population immediately; tests
    // use this after addOrganism to make hand-built organisms visible.
    void flushPending();

private:
    SimConfig settings;
    Rng random;
    World worldData;
    SpatialGrid spatialGrid;
    std::vector<std::unique_ptr<Organism>> population;
    std::vector<std::unique_ptr<Organism>> pending;
    uint32_t nextIdValue = 1;
    double simTime = 0.0;
};
