// Property tests for Split 4 reproduction, lineage, stats, and rescue. All
// randomness uses fixed seeds and assertions avoid exact random values.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

#include "core/Rng.h"
#include "genetics/Genetics.h"
#include "sim/Lineage.h"
#include "sim/Organism.h"
#include "sim/SimConfig.h"
#include "sim/Simulation.h"
#include "sim/Stats.h"

namespace {
constexpr float kTestSight = 400.f;
constexpr float kNoPlantPropagation = 1.0e9f;

// Isolated test world: no founders, no plant spread, no rescue, no upkeep.
SimConfig baseConfig() {
    SimConfig cfg;
    cfg.initialPlants = 0;
    cfg.initialHerbivores = 0;
    cfg.initialPredators = 0;
    cfg.plantSpawnMin = kNoPlantPropagation;
    cfg.plantSpawnMax = kNoPlantPropagation;
    cfg.baseUpkeep = 0.f;
    cfg.sightUpkeep = 0.f;
    cfg.herbivoreMaxAge = 1.0e9f;
    cfg.predatorMaxAge = 1.0e9f;
    cfg.enableRescue = false;
    cfg.genetics.sightMutationRate = 0.f;
    cfg.genetics.colourMutationRate = 0.f;
    return cfg;
}

// Builds a genome with both sight alleles set to the same value.
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

// Builds a hand-made herbivore with explicit energy and age.
std::unique_ptr<Herbivore> makeHerbivore(uint32_t id, const SimConfig& cfg,
                                         Rng& rng, glm::vec2 position,
                                         genetics::Sex sex, float energy,
                                         float age) {
    std::unique_ptr<Herbivore> herbivore = std::make_unique<Herbivore>(
        id, position, genomeWithSight(kTestSight, sex), cfg.herbivoreSpeed,
        cfg.herbivoreMaxAge, cfg, rng);
    herbivore->energy = energy;
    herbivore->age = age;
    return herbivore;
}

// Builds a hand-made predator with explicit energy and age.
std::unique_ptr<Predator> makePredator(uint32_t id, const SimConfig& cfg,
                                       Rng& rng, glm::vec2 position,
                                       genetics::Sex sex, float energy,
                                       float age) {
    std::unique_ptr<Predator> predator = std::make_unique<Predator>(
        id, position, genomeWithSight(kTestSight, sex), cfg.predatorSpeed,
        cfg.predatorMaxAge, cfg, rng);
    predator->energy = energy;
    predator->age = age;
    return predator;
}

// Adds a hand-made herbivore and returns its raw pointer for in-test checks.
Herbivore* addHerbivore(Simulation& sim, const SimConfig& cfg,
                        glm::vec2 position, genetics::Sex sex, float energy,
                        float age) {
    std::unique_ptr<Herbivore> herbivore =
        makeHerbivore(sim.nextId(), cfg, sim.rng(), position, sex, energy, age);
    Herbivore* raw = herbivore.get();
    sim.addOrganism(std::move(herbivore));
    return raw;
}

// Adds a hand-made predator and returns its raw pointer for in-test checks.
Predator* addPredator(Simulation& sim, const SimConfig& cfg,
                      glm::vec2 position, genetics::Sex sex, float energy,
                      float age) {
    std::unique_ptr<Predator> predator =
        makePredator(sim.nextId(), cfg, sim.rng(), position, sex, energy, age);
    Predator* raw = predator.get();
    sim.addOrganism(std::move(predator));
    return raw;
}

// Adds a hand-made plant and returns its raw pointer for in-test checks.
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

TEST_CASE("canMate respects age, energy, pregnancy, and cooldown") {
    const SimConfig cfg = baseConfig();
    Rng rng(4001);
    const glm::vec2 position(1500.f, 1500.f);

    std::unique_ptr<Herbivore> herbivore =
        makeHerbivore(1, cfg, rng, position, genetics::Sex::Female,
                      cfg.maxEnergy, 0.f);
    CHECK(!herbivore->isAdult(cfg));
    CHECK(!herbivore->canMate(cfg));

    herbivore->age = cfg.maturityAge;
    CHECK(herbivore->isAdult(cfg));
    CHECK(herbivore->canMate(cfg));

    herbivore->energy = cfg.mateEnergyThreshold * cfg.maxEnergy - 1.f;
    CHECK(!herbivore->canMate(cfg));

    herbivore->energy = cfg.maxEnergy;
    herbivore->embryos.push_back(herbivore->genome);
    CHECK(herbivore->pregnant());
    CHECK(!herbivore->canMate(cfg));

    herbivore->embryos.clear();
    herbivore->reproCooldown = 1.f;
    CHECK(!herbivore->canMate(cfg));
}

TEST_CASE("adult herbivores reproduce with inherited sight alleles") {
    const SimConfig cfg = baseConfig();
    Simulation sim(cfg, 4002);
    sim.populate();

    Herbivore* mother =
        addHerbivore(sim, cfg, {1495.f, 1500.f}, genetics::Sex::Female,
                     cfg.maxEnergy, cfg.maturityAge);
    Herbivore* father =
        addHerbivore(sim, cfg, {1505.f, 1500.f}, genetics::Sex::Male,
                     cfg.maxEnergy, cfg.maturityAge);
    const std::uint32_t motherId = mother->id;
    const std::uint32_t fatherId = father->id;
    const genetics::Genome motherGenome = mother->genome;
    const genetics::Genome fatherGenome = father->genome;
    sim.flushPending();

    REQUIRE(sim.find(motherId) != nullptr);
    REQUIRE(sim.find(fatherId) != nullptr);

    runSteps(sim, stepsForSeconds(cfg, 60.f));

    int offspringCount = 0;
    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        const Animal* child = dynamic_cast<const Animal*>(organism.get());
        if (child == nullptr || child->motherId != motherId ||
            child->fatherId != fatherId) {
            continue;
        }

        ++offspringCount;
        CHECK(child->generation == 1);
        CHECK((child->genome.sight[0] == motherGenome.sight[0] ||
               child->genome.sight[0] == motherGenome.sight[1]));
        CHECK((child->genome.sight[1] == fatherGenome.sight[0] ||
               child->genome.sight[1] == fatherGenome.sight[1]));
        CHECK(genetics::sexOf(child->genome) == child->sex);
    }
    CHECK(offspringCount >= 1);
}

TEST_CASE("lineage tracks parents, children, ancestry, and death") {
    const SimConfig cfg = baseConfig();
    Simulation sim(cfg, 4003);
    sim.populate();

    Herbivore* mother =
        addHerbivore(sim, cfg, {1495.f, 1500.f}, genetics::Sex::Female,
                     cfg.maxEnergy, cfg.maturityAge);
    Herbivore* father =
        addHerbivore(sim, cfg, {1505.f, 1500.f}, genetics::Sex::Male,
                     cfg.maxEnergy, cfg.maturityAge);
    const std::uint32_t motherId = mother->id;
    const std::uint32_t fatherId = father->id;
    sim.flushPending();

    runSteps(sim, stepsForSeconds(cfg, 60.f));

    const std::vector<std::uint32_t>& childIds =
        sim.lineage().children(motherId);
    REQUIRE(!childIds.empty());
    const std::uint32_t offspringId = childIds.front();

    CHECK(sim.lineage().find(motherId) != nullptr);
    CHECK(sim.lineage().find(fatherId) != nullptr);
    CHECK(sim.lineage().find(offspringId) != nullptr);
    CHECK(std::find(childIds.begin(), childIds.end(), offspringId) !=
          childIds.end());

    const std::vector<std::uint32_t> ancestors =
        sim.lineage().ancestors(offspringId, 2);
    CHECK(std::find(ancestors.begin(), ancestors.end(), motherId) !=
          ancestors.end());
    CHECK(std::find(ancestors.begin(), ancestors.end(), fatherId) !=
          ancestors.end());
    CHECK(sim.lineage().size() >= 3);

    Animal* offspring = dynamic_cast<Animal*>(sim.find(offspringId));
    REQUIRE(offspring != nullptr);
    offspring->alive = false;
    offspring->deathCause = DeathCause::Eaten;
    sim.step();

    const LineageRecord* record = sim.lineage().find(offspringId);
    REQUIRE(record != nullptr);
    CHECK(record->deathTime >= 0.0);
    CHECK(record->deathCause == DeathCause::Eaten);
}

TEST_CASE("same-sex herbivores do not reproduce") {
    const SimConfig cfg = baseConfig();
    Simulation sim(cfg, 4004);
    sim.populate();

    addHerbivore(sim, cfg, {1495.f, 1500.f}, genetics::Sex::Male,
                 cfg.maxEnergy, cfg.maturityAge);
    addHerbivore(sim, cfg, {1505.f, 1500.f}, genetics::Sex::Male,
                 cfg.maxEnergy, cfg.maturityAge);
    sim.flushPending();

    runSteps(sim, stepsForSeconds(cfg, 60.f));
    CHECK(sim.birthCount() == 0);
    CHECK(sim.count(Kind::Herbivore) == 2);
}

TEST_CASE("population cap prevents conception") {
    SimConfig cfg = baseConfig();
    cfg.maxHerbivores = 2;
    Simulation sim(cfg, 4005);
    sim.populate();

    addHerbivore(sim, cfg, {1495.f, 1500.f}, genetics::Sex::Female,
                 cfg.maxEnergy, cfg.maturityAge);
    addHerbivore(sim, cfg, {1505.f, 1500.f}, genetics::Sex::Male,
                 cfg.maxEnergy, cfg.maturityAge);
    sim.flushPending();

    runSteps(sim, stepsForSeconds(cfg, 60.f));
    CHECK(sim.birthCount() == 0);
    CHECK(sim.count(Kind::Herbivore) == 2);
}

TEST_CASE("stats samples match counts and CSV has one row per sample") {
    const SimConfig cfg = baseConfig();
    Simulation sim(cfg, 4006);
    sim.populate();

    addPlant(sim, cfg, {1500.f, 1500.f});
    addHerbivore(sim, cfg, {1520.f, 1500.f}, genetics::Sex::Female,
                 cfg.maxEnergy, 0.f);
    addPredator(sim, cfg, {1540.f, 1500.f}, genetics::Sex::Male,
                cfg.maxEnergy, 0.f);
    sim.flushPending();

    runSteps(sim, stepsForSeconds(cfg, 1.f));
    sim.stats().sample(sim);

    const std::vector<StatsSample>& samples = sim.stats().samples();
    REQUIRE(samples.size() >= 2);
    const StatsSample& latest = samples.back();
    CHECK(latest.plants == sim.count(Kind::Plant));
    CHECK(latest.herbivores.count == sim.count(Kind::Herbivore));
    CHECK(latest.predators.count == sim.count(Kind::Predator));

    for (std::size_t i = 1; i < samples.size(); ++i) {
        CHECK(samples[i].time > samples[i - 1].time);
    }

    const std::filesystem::path csvPath =
        std::filesystem::temp_directory_path() /
        "ecosim_reproduction_stats.csv";
    REQUIRE(sim.stats().writeCsv(csvPath.string()));

    std::ifstream input(csvPath.string());
    REQUIRE(input.good());
    std::string header;
    REQUIRE(std::getline(input, header));
    CHECK(header.find("time,plants") == 0);

    int dataLines = 0;
    std::string line;
    while (std::getline(input, line)) {
        ++dataLines;
    }
    CHECK(dataLines == static_cast<int>(samples.size()));
    input.close();
    std::filesystem::remove(csvPath);
}

TEST_CASE("rescue restores underpopulated kinds") {
    SimConfig cfg = baseConfig();
    cfg.initialHerbivores = 2;
    cfg.enableRescue = true;
    cfg.rescueFloor = 6;
    cfg.rescueInterval = 10.f;

    Simulation sim(cfg, 4007);
    sim.populate();
    runSteps(sim, stepsForSeconds(cfg, 25.f));

    CHECK(sim.count(Kind::Herbivore) >= 4);
    CHECK(sim.immigrantCount() >= 2);
}
