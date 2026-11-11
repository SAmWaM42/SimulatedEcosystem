// Simulation manager: owns the population, the shared RNG, the world, and the
// spatial grid, and advances all organisms one fixed timestep at a time.
#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>

#include <glm/vec2.hpp>

#include "core/Rng.h"
#include "sim/Lineage.h"
#include "sim/Organism.h"
#include "sim/SimConfig.h"
#include "sim/SpatialGrid.h"
#include "sim/Stats.h"
#include "sim/World.h"

// Owns every organism and coordinates population updates, births, and death.
class Simulation {
public:
    // Builds a simulation with the given settings and deterministic RNG seed.
    Simulation(const SimConfig& cfg, uint32_t seed,
               const Terrain* terrain = nullptr);

    // Creates the configured founder plants and animals at walkable positions.
    void populate();

    // Advances the whole simulation by exactly cfg.fixedDt seconds.
    // Saves interpolation state for every living organism before updates.
    void step();

    // Queues an organism; it joins the main population at the end of the step.
    void addOrganism(std::unique_ptr<Organism> o);

    // Returns the next unique organism id, starting at 1.
    uint32_t nextId();

    // Returns the immutable settings shared by every organism.
    const SimConfig& config() const;
    // Returns the mutable settings so the UI can apply live tunables.
    SimConfig& tunables();
    // Returns the shared RNG so behaviour and tests draw from one stream.
    Rng& rng();
    // Returns the world used for containment and clamping.
    const World& world() const;
    // Returns the grid rebuilt at the start of each step for proximity queries.
    SpatialGrid& grid();
    // Returns the simulated time in seconds.
    double time() const;
    // Returns a random point inside the world that is not water; retries 200
    // times, then falls back to the world centre.
    glm::vec2 randomWalkablePosition();
    // Returns the current population, excluding organisms still queued.
    const std::vector<std::unique_ptr<Organism>>& organisms() const;

    // Living organisms of a kind, including additions queued this step so the
    // plant cap sees every birth before it is inserted into the grid.
    int count(Kind k) const;

    // Mean cached sight for an animal kind, including queued additions, or 0
    // when none of that kind live.
    float meanSight(Kind k) const;

    // Returns the living organism with this id, or nullptr; the lookup is
    // rebuilt at the start of every step so behaviour never stores raw pointers.
    Organism* find(uint32_t id) const;

    // Returns the cumulative number of removed organisms of a kind and cause.
    int deathCount(Kind kind, DeathCause cause) const;

    // Returns the retained family tree and its mutation interface.
    LineageTracker& lineage();
    const LineageTracker& lineage() const;
    // Returns the stats collector and its mutation interface.
    StatsManager& stats();
    const StatsManager& stats() const;

    // Increments the cumulative reproduction birth counter (once per child).
    void noteBirth();
    // Cumulative number of offspring born through reproduction.
    int birthCount() const;
    // Cumulative number of rescue immigrants spawned.
    int immigrantCount() const;

    // Moves all queued organisms into the main population immediately; tests
    // use this after addOrganism to make hand-built organisms visible.
    void flushPending();

private:
    // Spawns one immigrant for each animal kind below the rescue floor.
    void spawnRescueImmigrants();

    SimConfig settings;
    Rng random;
    World worldData;
    SpatialGrid spatialGrid;
    LineageTracker lineageTracker;
    StatsManager statsManager;
    std::vector<std::unique_ptr<Organism>> population;
    std::vector<std::unique_ptr<Organism>> pending;
    std::unordered_map<uint32_t, Organism*> byId;  // rebuilt each step
    int deathCounts[3][4] = {};  // [Kind][DeathCause], cumulative
    uint32_t nextIdValue = 1;
    double simTime = 0.0;
    double nextRescueTime = 0.0;
    double nextStatsTime = 0.0;
    int birthCounter = 0;
    int immigrantCounter = 0;
};

