// Headless property tests for Split 6: clock timing, palette separation,
// terrain-aware simulation, interpolation, screen rays, and picking.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "core/Rng.h"
#include "genetics/Genetics.h"
#include "render/Camera.h"
#include "render/Palette.h"
#include "render/Picking.h"
#include "sim/Organism.h"
#include "sim/SimClock.h"
#include "sim/SimConfig.h"
#include "sim/Simulation.h"
#include "world/Terrain.h"

namespace {
// Empty world for hand-placed picking tests.
SimConfig emptyConfig() {
    SimConfig cfg;
    cfg.initialPlants = 0;
    cfg.initialHerbivores = 0;
    cfg.initialPredators = 0;
    cfg.plantSpawnMin = 1.0e9f;
    cfg.plantSpawnMax = 1.0e9f;
    cfg.enableRescue = false;
    return cfg;
}

// Builds an explicit herbivore genome for the picking test.
genetics::Genome testGenome() {
    genetics::Genome genome;
    genome.sight = {400.f, 400.f};
    genome.colour = {genetics::ColourAllele::Medium,
                     genetics::ColourAllele::Light};
    genome.sexChromosomes = {genetics::SexChromosome::X,
                             genetics::SexChromosome::X};
    return genome;
}
}  // namespace

TEST_CASE("SimClock advances and clamps correctly") {
    SimClock realistic(1.f / 30.f);
    int steps = 0;
    for (int frame = 0; frame < 60; ++frame) {
        steps += realistic.advance(1.f / 60.f);
    }
    CHECK(steps >= 28);
    CHECK(steps <= 32);

    SimClock fast(1.f / 30.f);
    fast.setSpeed(4.f);
    steps = 0;
    for (int frame = 0; frame < 60; ++frame) {
        steps += fast.advance(1.f / 60.f);
    }
    CHECK(steps >= 118);
    CHECK(steps <= 122);

    SimClock paused(1.f / 30.f);
    paused.setPaused(true);
    const float alphaBefore = paused.alpha();
    CHECK(paused.advance(1.f) == 0);
    CHECK(paused.alpha() == doctest::Approx(alphaBefore));

    SimClock clamped(1.f / 30.f);
    clamped.setSpeed(0.f);
    CHECK(clamped.speed() == doctest::Approx(0.25f));
    clamped.setSpeed(100.f);
    CHECK(clamped.speed() == doctest::Approx(16.f));

    SimClock huge(1.f / 30.f, 20);
    CHECK(huge.advance(100.f) == 20);
    CHECK(huge.alpha() < 1.f);
}

TEST_CASE("animal palette colours are distinguishable") {
    const Kind kinds[2] = {Kind::Herbivore, Kind::Predator};
    for (Kind kind : kinds) {
        const glm::vec3 dark = organismColour(
            kind, genetics::ColourPhenotype::Dark);
        const glm::vec3 medium = organismColour(
            kind, genetics::ColourPhenotype::Medium);
        const glm::vec3 light = organismColour(
            kind, genetics::ColourPhenotype::Light);
        CHECK(glm::length(dark - medium) >= 0.1f);
        CHECK(glm::length(dark - light) >= 0.1f);
        CHECK(glm::length(medium - light) >= 0.1f);
    }
}

TEST_CASE("terrain-aware simulation never puts organisms in water") {
    TerrainConfig terrainConfig;
    terrainConfig.seed = 1;
    const Terrain terrain(terrainConfig);

    const SimConfig cfg;
    Simulation sim(cfg, 42, &terrain);
    sim.populate();

    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        CHECK(!terrain.isWater(organism->position.x, organism->position.y));
    }

    const int steps =
        static_cast<int>(120.f / cfg.fixedDt + 0.5f);
    for (int step = 0; step < steps; ++step) {
        sim.step();
    }

    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        if (organism->alive) {
            CHECK(!terrain.isWater(organism->position.x,
                                   organism->position.y));
        }
    }
}

TEST_CASE("interpolation distance is bounded by one movement step") {
    TerrainConfig terrainConfig;
    terrainConfig.seed = 2;
    const Terrain terrain(terrainConfig);

    const SimConfig cfg;
    Simulation sim(cfg, 7, &terrain);
    sim.populate();
    sim.step();

    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        const Animal* animal =
            dynamic_cast<const Animal*>(organism.get());
        if (animal == nullptr || !animal->alive) {
            continue;
        }
        const float distance =
            glm::length(animal->prevPosition - animal->position);
        CHECK(distance <= animal->speed * cfg.fixedDt + 1e-3f);
    }
}

TEST_CASE("camera centre ray reaches the target") {
    TerrainConfig terrainConfig;
    const Terrain terrain(terrainConfig);
    const Camera camera(terrain);

    const Ray ray = camera.screenRay(glm::vec2(400.f, 300.f), 800, 600);
    CHECK(glm::length(ray.direction) == doctest::Approx(1.f));
    const glm::vec3 toTarget = camera.target() - ray.origin;
    CHECK(glm::length(glm::cross(toTarget, ray.direction)) <= 1.f);
}

TEST_CASE("picking chooses the nearest organism") {
    const SimConfig cfg = emptyConfig();
    TerrainConfig terrainConfig;
    terrainConfig.seed = 3;
    const Terrain terrain(terrainConfig);

    Simulation sim(cfg, 11, &terrain);
    sim.populate();

    const glm::vec2 herbivorePosition(1000.f, 1000.f);
    const glm::vec2 plantPosition(1200.f, 1000.f);
    std::unique_ptr<Herbivore> herbivore = std::make_unique<Herbivore>(
        sim.nextId(), herbivorePosition, testGenome(), cfg.herbivoreSpeed,
        cfg.herbivoreMaxAge, cfg, sim.rng());
    const std::uint32_t herbivoreId = herbivore->id;
    std::unique_ptr<Plant> plant = std::make_unique<Plant>(
        sim.nextId(), plantPosition, cfg, sim.rng());

    sim.addOrganism(std::move(herbivore));
    sim.addOrganism(std::move(plant));
    sim.flushPending();

    const glm::vec3 herbivoreCentre(
        herbivorePosition.x,
        terrain.heightAt(herbivorePosition.x, herbivorePosition.y) +
            1.2f * cfg.bodyRadius,
        herbivorePosition.y);
    const glm::vec3 plantCentre(
        plantPosition.x,
        terrain.heightAt(plantPosition.x, plantPosition.y) +
            1.2f * cfg.bodyRadius,
        plantPosition.y);

    Ray hitRay;
    hitRay.direction = glm::normalize(plantCentre - herbivoreCentre);
    hitRay.origin = herbivoreCentre - hitRay.direction * 100.f;
    CHECK(pickOrganism(hitRay, sim, terrain) == herbivoreId);

    Ray missRay;
    missRay.origin = herbivoreCentre + glm::vec3(0.f, 1000.f, 0.f);
    missRay.direction = glm::vec3(0.f, 1.f, 0.f);
    CHECK(pickOrganism(missRay, sim, terrain) == 0);
}
