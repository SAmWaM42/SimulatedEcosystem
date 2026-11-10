// Property tests for the headless simulation: spatial queries, founder setup,
// plant growth, movement bounds, starvation, sight upkeep, and determinism.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

#include "core/Rng.h"
#include "genetics/Genetics.h"
#include "sim/Organism.h"
#include "sim/SimConfig.h"
#include "sim/Simulation.h"
#include "sim/SpatialGrid.h"
#include "sim/World.h"

namespace {
// Builds a genome by hand for deterministic animal tests.
genetics::Genome makeGenome(
    std::array<float, 2> sightAlleles,
    std::array<genetics::ColourAllele, 2> colourAlleles,
    std::array<genetics::SexChromosome, 2> chromosomes) {
    genetics::Genome genome;
    genome.sight = sightAlleles;
    genome.colour = colourAlleles;
    genome.sexChromosomes = chromosomes;
    return genome;
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

TEST_CASE("spatial grid matches brute-force radius queries") {
    constexpr float kWorldSize = 3000.f;
    constexpr float kCellSize = 200.f;
    SimConfig cfg;
    Rng rng(101);

    std::vector<std::unique_ptr<Plant>> plants;
    plants.reserve(500);
    SpatialGrid grid(kWorldSize, kCellSize);
    for (int i = 0; i < 500; ++i) {
        plants.push_back(std::make_unique<Plant>(
            static_cast<std::uint32_t>(i + 1),
            glm::vec2(rng.uniform(0.f, kWorldSize),
                      rng.uniform(0.f, kWorldSize)),
            cfg, rng));
        grid.insert(plants.back().get());
    }

    for (int query = 0; query < 20; ++query) {
        const glm::vec2 centre(rng.uniform(0.f, kWorldSize),
                               rng.uniform(0.f, kWorldSize));
        const float radius = rng.uniform(0.f, 400.f);

        std::vector<Organism*> found;
        grid.queryRadius(centre, radius, found);

        std::set<std::uint32_t> foundIds;
        for (const Organism* organism : found) {
            foundIds.insert(organism->id);
        }

        std::set<std::uint32_t> expectedIds;
        const float radiusSquared = radius * radius;
        for (const std::unique_ptr<Plant>& plant : plants) {
            const glm::vec2 offset = plant->position - centre;
            if (glm::dot(offset, offset) <= radiusSquared) {
                expectedIds.insert(plant->id);
            }
        }

        CHECK(foundIds.size() == expectedIds.size());
        for (std::uint32_t id : expectedIds) {
            CHECK(foundIds.count(id) == 1);
        }
    }
}

TEST_CASE("populate creates valid founders") {
    const SimConfig cfg;
    Simulation sim(cfg, 202);
    sim.populate();

    CHECK(sim.count(Kind::Plant) == cfg.initialPlants);
    CHECK(sim.count(Kind::Herbivore) == cfg.initialHerbivores);
    CHECK(sim.count(Kind::Predator) == cfg.initialPredators);
    CHECK(sim.organisms().size() ==
          static_cast<std::size_t>(cfg.initialPlants + cfg.initialHerbivores +
                                   cfg.initialPredators));

    std::set<std::uint32_t> ids;
    int femaleHerbivores = 0;
    int maleHerbivores = 0;
    int femalePredators = 0;
    int malePredators = 0;

    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        CHECK(organism->id != 0);
        const bool inserted = ids.insert(organism->id).second;
        CHECK(inserted);
        CHECK(sim.world().inside(organism->position));

        if (organism->kind() == Kind::Plant) {
            continue;
        }

        const Animal* animal = dynamic_cast<const Animal*>(organism.get());
        REQUIRE(animal != nullptr);
        CHECK(animal->sight >= cfg.genetics.sightMin);
        CHECK(animal->sight <= cfg.genetics.sightMax);

        if (organism->kind() == Kind::Herbivore) {
            if (animal->sex == genetics::Sex::Female) {
                ++femaleHerbivores;
            } else {
                ++maleHerbivores;
            }
        } else {
            if (animal->sex == genetics::Sex::Female) {
                ++femalePredators;
            } else {
                ++malePredators;
            }
        }
    }

    CHECK(femaleHerbivores == cfg.initialHerbivores / 2);
    CHECK(maleHerbivores == cfg.initialHerbivores / 2);
    CHECK(femalePredators == cfg.initialPredators / 2);
    CHECK(malePredators == cfg.initialPredators / 2);
    CHECK(ids.size() == sim.organisms().size());
}

TEST_CASE("plants grow but never exceed the cap") {
    SimConfig cfg;
    cfg.initialHerbivores = 0;
    cfg.initialPredators = 0;
    Simulation sim(cfg, 303);
    sim.populate();

    const int initialPlantCount = sim.count(Kind::Plant);
    CHECK(initialPlantCount == cfg.initialPlants);

    const int steps = stepsForSeconds(cfg, 300.f);
    bool withinCap = true;
    for (int i = 0; i < steps; ++i) {
        sim.step();
        if (sim.count(Kind::Plant) > cfg.maxPlants) {
            withinCap = false;
        }
    }

    CHECK(withinCap);
    CHECK(sim.count(Kind::Plant) > initialPlantCount);

    bool allInside = true;
    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        if (!sim.world().inside(organism->position)) {
            allInside = false;
        }
    }
    CHECK(allInside);
}

TEST_CASE("animals stay inside the world") {
    SimConfig cfg;
    cfg.initialPlants = 0;
    cfg.baseUpkeep = 0.f;
    cfg.sightUpkeep = 0.f;
    cfg.herbivoreMaxAge = 1.0e9f;
    cfg.predatorMaxAge = 1.0e9f;

    Simulation sim(cfg, 404);
    sim.populate();

    const int steps = stepsForSeconds(cfg, 300.f);
    bool allInside = true;
    for (int step = 0; step < steps; ++step) {
        sim.step();
        for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
            if (!sim.world().inside(organism->position)) {
                allInside = false;
            }
        }
    }

    CHECK(allInside);
    CHECK(sim.count(Kind::Herbivore) == cfg.initialHerbivores);
    CHECK(sim.count(Kind::Predator) == cfg.initialPredators);
}

TEST_CASE("animals starve without plants") {
    SimConfig cfg;
    cfg.initialPlants = 0;
    Simulation sim(cfg, 505);
    sim.populate();

    runSteps(sim, stepsForSeconds(cfg, 200.f));

    CHECK(sim.count(Kind::Herbivore) == 0);
    CHECK(sim.count(Kind::Predator) == 0);
    CHECK(sim.organisms().empty());
}

TEST_CASE("sight costs energy") {
    SimConfig cfg;
    cfg.initialPlants = 0;
    Simulation sim(cfg, 606);

    const genetics::Genome lowSightGenome =
        makeGenome({200.f, 200.f},
                   {genetics::ColourAllele::Medium, genetics::ColourAllele::Light},
                   {genetics::SexChromosome::X, genetics::SexChromosome::X});
    const genetics::Genome highSightGenome =
        makeGenome({400.f, 400.f},
                   {genetics::ColourAllele::Dark, genetics::ColourAllele::Light},
                   {genetics::SexChromosome::X, genetics::SexChromosome::Y});

    std::unique_ptr<Herbivore> lowSight =
        std::make_unique<Herbivore>(sim.nextId(), glm::vec2(1500.f, 1500.f),
                                    lowSightGenome, cfg.herbivoreSpeed,
                                    cfg.herbivoreMaxAge, cfg, sim.rng());
    std::unique_ptr<Herbivore> highSight =
        std::make_unique<Herbivore>(sim.nextId(), glm::vec2(1500.f, 1500.f),
                                    highSightGenome, cfg.herbivoreSpeed,
                                    cfg.herbivoreMaxAge, cfg, sim.rng());
    Herbivore* lowSightPtr = lowSight.get();
    Herbivore* highSightPtr = highSight.get();

    sim.addOrganism(std::move(lowSight));
    sim.addOrganism(std::move(highSight));
    sim.flushPending();
    REQUIRE(sim.organisms().size() == 2);

    runSteps(sim, stepsForSeconds(cfg, 10.f));

    REQUIRE(lowSightPtr->alive);
    REQUIRE(highSightPtr->alive);
    CHECK(highSightPtr->energy < lowSightPtr->energy);

    const float expectedDifference = cfg.sightUpkeep * 200.f * 10.f;
    const float actualDifference = lowSightPtr->energy - highSightPtr->energy;
    CHECK(actualDifference == doctest::Approx(expectedDifference).epsilon(0.01));
}

TEST_CASE("same seed and config are deterministic") {
    SimConfig cfg;
    Simulation first(cfg, 707);
    Simulation second(cfg, 707);
    first.populate();
    second.populate();

    runSteps(first, 100);
    runSteps(second, 100);

    CHECK(first.count(Kind::Plant) == second.count(Kind::Plant));
    CHECK(first.count(Kind::Herbivore) == second.count(Kind::Herbivore));
    CHECK(first.count(Kind::Predator) == second.count(Kind::Predator));

    const std::vector<std::unique_ptr<Organism>>& firstOrganisms =
        first.organisms();
    const std::vector<std::unique_ptr<Organism>>& secondOrganisms =
        second.organisms();
    REQUIRE(firstOrganisms.size() >= 10);
    REQUIRE(secondOrganisms.size() >= 10);

    for (std::size_t i = 0; i < 10; ++i) {
        CHECK(firstOrganisms[i]->id == secondOrganisms[i]->id);
        CHECK(firstOrganisms[i]->position.x == secondOrganisms[i]->position.x);
        CHECK(firstOrganisms[i]->position.y == secondOrganisms[i]->position.y);
    }
}
