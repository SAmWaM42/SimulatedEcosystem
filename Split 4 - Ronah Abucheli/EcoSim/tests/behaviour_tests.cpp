// Property tests for Split 3 behaviour: perception probability, eating,
// hunger gating, fleeing, predation, misses, and a default-config smoke test.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cstdint>
#include <memory>
#include <utility>

#include <glm/vec2.hpp>

#include "genetics/Genetics.h"
#include "sim/Organism.h"
#include "sim/Perception.h"
#include "sim/SimConfig.h"
#include "sim/Simulation.h"

namespace {
constexpr float kTestSight = 400.f;
constexpr float kNoPlantPropagation = 1.0e9f;

// Empty world with founders disabled and plants effectively unable to spread.
SimConfig baseConfig() {
    SimConfig cfg;
    cfg.initialPlants = 0;
    cfg.initialHerbivores = 0;
    cfg.initialPredators = 0;
    cfg.plantSpawnMin = kNoPlantPropagation;
    cfg.plantSpawnMax = kNoPlantPropagation;
    cfg.enableRescue = false;  // isolate behaviour from the demo safety net
    cfg.predatorPreyRefuge = 0;  // allow predation tests with a single prey
    return cfg;
}

// Builds an explicit genome with both sight alleles set to the same value.
genetics::Genome genomeWithSight(float sight, genetics::Sex sex) {
    genetics::Genome genome;
    genome.sight = {sight, sight};
    genome.colour = {genetics::ColourAllele::Medium,
                     genetics::ColourAllele::Light};
    if (sex == genetics::Sex::Female) {
        genome.sexChromosomes = {genetics::SexChromosome::X,
                                 genetics::SexChromosome::X};
    } else {
        genome.sexChromosomes = {genetics::SexChromosome::X,
                                 genetics::SexChromosome::Y};
    }
    return genome;
}

// Adds a hand-built herbivore and returns its raw pointer for in-test checks.
Herbivore* addHerbivore(Simulation& sim, const SimConfig& cfg,
                        glm::vec2 position, float energy) {
    std::unique_ptr<Herbivore> herbivore = std::make_unique<Herbivore>(
        sim.nextId(), position,
        genomeWithSight(kTestSight, genetics::Sex::Female),
        cfg.herbivoreSpeed, cfg.herbivoreMaxAge, cfg, sim.rng());
    herbivore->energy = energy;
    Herbivore* raw = herbivore.get();
    sim.addOrganism(std::move(herbivore));
    return raw;
}

// Adds a hand-built predator and returns its raw pointer for in-test checks.
Predator* addPredator(Simulation& sim, const SimConfig& cfg,
                      glm::vec2 position, float energy) {
    std::unique_ptr<Predator> predator = std::make_unique<Predator>(
        sim.nextId(), position,
        genomeWithSight(kTestSight, genetics::Sex::Male),
        cfg.predatorSpeed, cfg.predatorMaxAge, cfg, sim.rng());
    predator->energy = energy;
    Predator* raw = predator.get();
    sim.addOrganism(std::move(predator));
    return raw;
}

// Adds a hand-built plant and returns its raw pointer for in-test checks.
Plant* addPlant(Simulation& sim, const SimConfig& cfg, glm::vec2 position) {
    std::unique_ptr<Plant> plant =
        std::make_unique<Plant>(sim.nextId(), position, cfg, sim.rng());
    Plant* raw = plant.get();
    sim.addOrganism(std::move(plant));
    return raw;
}

// Converts a simulated duration to the nearest whole number of fixed steps.
int stepsForSeconds(const SimConfig& cfg, float seconds) {
    return static_cast<int>(seconds / cfg.fixedDt + 0.5f);
}

// Advances a simulation by a whole number of fixed steps.
void runSteps(Simulation& sim, int steps) {
    for (int i = 0; i < steps; ++i) {
        sim.step();
    }
}
}  // namespace

TEST_CASE("detection chance decreases linearly with distance") {
    constexpr float sight = 400.f;
    CHECK(detectionChance(0.f, sight) == doctest::Approx(1.f));
    CHECK(detectionChance(sight * 0.5f, sight) == doctest::Approx(0.5f));
    CHECK(detectionChance(sight, sight) == 0.f);
    CHECK(detectionChance(sight + 50.f, sight) == 0.f);
    CHECK(detectionChance(50.f, 0.f) == 0.f);

    float previous = detectionChance(0.f, sight);
    for (float distance = 0.f; distance <= sight + 100.f; distance += 10.f) {
        const float current = detectionChance(distance, sight);
        CHECK(current <= previous);
        previous = current;
    }
}

TEST_CASE("hungry herbivore eats a nearby plant") {
    const SimConfig cfg = baseConfig();
    Simulation sim(cfg, 2001);
    sim.populate();

    Herbivore* herbivore = addHerbivore(sim, cfg, {1450.f, 1500.f}, 40.f);
    Plant* plant = addPlant(sim, cfg, {1550.f, 1500.f});
    sim.flushPending();
    REQUIRE(herbivore->alive);
    REQUIRE(plant->alive);

    runSteps(sim, stepsForSeconds(cfg, 30.f));
    CHECK(sim.deathCount(Kind::Plant, DeathCause::Eaten) == 1);
}

TEST_CASE("full herbivore ignores a nearby plant") {
    SimConfig cfg = baseConfig();
    cfg.baseUpkeep = 0.f;
    cfg.sightUpkeep = 0.f;
    Simulation sim(cfg, 2002);
    sim.populate();

    Herbivore* herbivore = addHerbivore(sim, cfg, {1500.f, 1500.f},
                                        cfg.maxEnergy);
    Plant* plant = addPlant(sim, cfg, {1550.f, 1500.f});
    const std::uint32_t plantId = plant->id;
    sim.flushPending();
    REQUIRE(herbivore->alive);
    REQUIRE(plant->alive);

    runSteps(sim, stepsForSeconds(cfg, 10.f));
    CHECK(sim.deathCount(Kind::Plant, DeathCause::Eaten) == 0);
    CHECK(sim.find(plantId) != nullptr);
}

TEST_CASE("herbivore enters flee state near a predator") {
    const SimConfig cfg = baseConfig();
    Simulation sim(cfg, 2003);
    sim.populate();

    Herbivore* herbivore = addHerbivore(sim, cfg, {1500.f, 1500.f},
                                        cfg.maxEnergy);
    Predator* predator = addPredator(sim, cfg, {1600.f, 1500.f},
                                     cfg.maxEnergy);
    sim.flushPending();
    REQUIRE(herbivore->alive);
    REQUIRE(predator->alive);

    bool sawFlee = false;
    const int steps = stepsForSeconds(cfg, 5.f);
    for (int i = 0; i < steps && !sawFlee; ++i) {
        sim.step();
        if (herbivore->state() == State::Flee) {
            sawFlee = true;
        }
    }
    CHECK(sawFlee);
}

TEST_CASE("predator catches a herbivore") {
    SimConfig cfg = baseConfig();
    cfg.predatorCatchChance = 1.f;
    Simulation sim(cfg, 2004);
    sim.populate();

    Predator* predator = addPredator(sim, cfg, {1500.f, 1500.f}, 70.f);
    Herbivore* herbivore = addHerbivore(sim, cfg, {1560.f, 1500.f},
                                        cfg.startEnergy);
    const std::uint32_t herbivoreId = herbivore->id;
    sim.flushPending();
    REQUIRE(predator->alive);
    REQUIRE(herbivore->alive);

    runSteps(sim, stepsForSeconds(cfg, 60.f));
    CHECK(sim.deathCount(Kind::Herbivore, DeathCause::Eaten) == 1);
    CHECK(sim.find(herbivoreId) == nullptr);
}

TEST_CASE("misses leave the prey alive") {
    SimConfig cfg = baseConfig();
    cfg.predatorCatchChance = 0.f;
    cfg.baseUpkeep = 0.f;
    cfg.sightUpkeep = 0.f;
    Simulation sim(cfg, 2005);
    sim.populate();

    Predator* predator = addPredator(sim, cfg, {1500.f, 1500.f}, 70.f);
    Herbivore* herbivore = addHerbivore(sim, cfg, {1560.f, 1500.f},
                                        cfg.startEnergy);
    const std::uint32_t herbivoreId = herbivore->id;
    sim.flushPending();
    REQUIRE(predator->alive);
    REQUIRE(herbivore->alive);

    runSteps(sim, stepsForSeconds(cfg, 30.f));
    CHECK(sim.find(herbivoreId) != nullptr);
}

TEST_CASE("default simulation remains populated after 100 seconds") {
    const SimConfig cfg;
    Simulation sim(cfg, 2006);
    sim.populate();

    runSteps(sim, stepsForSeconds(cfg, 100.f));
    CHECK(sim.count(Kind::Herbivore) > 0);
    CHECK(sim.count(Kind::Plant) > 0);
}
