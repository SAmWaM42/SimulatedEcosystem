// Headless tests for Split 7 UI support: enum display names, clock formatting,
// live-tunable copying, and chart-series preparation.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "sim/Organism.h"
#include "sim/SimConfig.h"
#include "sim/Simulation.h"
#include "sim/Stats.h"
#include "ui/ChartData.h"
#include "ui/UiText.h"

namespace {
// Checks that all values produce non-empty, unique display names.
template <typename Values, typename NameFn>
void checkUniqueNames(const Values& values, NameFn nameFn) {
    std::set<std::string> names;
    for (const auto& value : values) {
        const std::string name = nameFn(value);
        CHECK_FALSE(name.empty());
        CHECK(names.insert(name).second);
    }
}

// Checks every field that copyLiveTunables must leave untouched.
void checkNonLiveFieldsUnchanged(const SimConfig& before,
                                 const SimConfig& after) {
    CHECK(after.worldSize == before.worldSize);
    CHECK(after.fixedDt == before.fixedDt);
    CHECK(after.boundaryMargin == before.boundaryMargin);
    CHECK(after.spatialCellSize == before.spatialCellSize);
    CHECK(after.initialPlants == before.initialPlants);
    CHECK(after.maxPlants == before.maxPlants);
    CHECK(after.plantSpreadRadius == before.plantSpreadRadius);
    CHECK(after.plantCrowdRadius == before.plantCrowdRadius);
    CHECK(after.plantMaxNeighbours == before.plantMaxNeighbours);
    CHECK(after.maxEnergy == before.maxEnergy);
    CHECK(after.startEnergy == before.startEnergy);
    CHECK(after.baseUpkeep == before.baseUpkeep);
    CHECK(after.bodyRadius == before.bodyRadius);
    CHECK(after.wanderTurnMin == before.wanderTurnMin);
    CHECK(after.wanderTurnMax == before.wanderTurnMax);
    CHECK(after.wanderTurnAngle == before.wanderTurnAngle);
    CHECK(after.senseInterval == before.senseInterval);
    CHECK(after.hungerThreshold == before.hungerThreshold);
    CHECK(after.eatReach == before.eatReach);
    CHECK(after.preyEnergyValue == before.preyEnergyValue);
    CHECK(after.predatorMissCooldown == before.predatorMissCooldown);
    CHECK(after.maturityAge == before.maturityAge);
    CHECK(after.mateEnergyThreshold == before.mateEnergyThreshold);
    CHECK(after.mateReach == before.mateReach);
    CHECK(after.gestationTime == before.gestationTime);
    CHECK(after.litterMin == before.litterMin);
    CHECK(after.litterMax == before.litterMax);
    CHECK(after.pregnancyEnergyCost == before.pregnancyEnergyCost);
    CHECK(after.matingEnergyCost == before.matingEnergyCost);
    CHECK(after.postBirthCooldown == before.postBirthCooldown);
    CHECK(after.maleCooldown == before.maleCooldown);
    CHECK(after.offspringStartEnergy == before.offspringStartEnergy);
    CHECK(after.birthScatter == before.birthScatter);
    CHECK(after.maxHerbivores == before.maxHerbivores);
    CHECK(after.maxPredators == before.maxPredators);
    CHECK(after.rescueFloor == before.rescueFloor);
    CHECK(after.rescueInterval == before.rescueInterval);
    CHECK(after.statsInterval == before.statsInterval);
    CHECK(after.initialHerbivores == before.initialHerbivores);
    CHECK(after.herbivoreSpeed == before.herbivoreSpeed);
    CHECK(after.herbivoreMaxAge == before.herbivoreMaxAge);
    CHECK(after.initialPredators == before.initialPredators);
    CHECK(after.predatorSpeed == before.predatorSpeed);
    CHECK(after.predatorMaxAge == before.predatorMaxAge);
    CHECK(after.predatorUpkeepMultiplier ==
          before.predatorUpkeepMultiplier);
    CHECK(after.predatorStartEnergy == before.predatorStartEnergy);
    CHECK(after.predatorSightMultiplier == before.predatorSightMultiplier);
    CHECK(after.predatorPreyRefuge == before.predatorPreyRefuge);
    CHECK(after.genetics.sightMin == before.genetics.sightMin);
    CHECK(after.genetics.sightMax == before.genetics.sightMax);
}
}  // namespace

TEST_CASE("UI text covers every enum value and formats time") {
    const std::array<Kind, 3> kinds = {
        Kind::Plant, Kind::Herbivore, Kind::Predator};
    checkUniqueNames(kinds, [](Kind value) { return kindName(value); });

    const std::array<State, 4> states = {
        State::Wander, State::SeekFood, State::Flee, State::SeekMate};
    checkUniqueNames(states, [](State value) { return stateName(value); });

    const std::array<genetics::ColourPhenotype, 3> phenotypes = {
        genetics::ColourPhenotype::Dark,
        genetics::ColourPhenotype::Medium,
        genetics::ColourPhenotype::Light};
    checkUniqueNames(
        phenotypes,
        [](genetics::ColourPhenotype value) { return phenotypeName(value); });

    const std::array<genetics::ColourAllele, 3> alleles = {
        genetics::ColourAllele::Dark, genetics::ColourAllele::Medium,
        genetics::ColourAllele::Light};
    checkUniqueNames(
        alleles,
        [](genetics::ColourAllele value) { return alleleName(value); });

    const std::array<genetics::SexChromosome, 2> chromosomes = {
        genetics::SexChromosome::X, genetics::SexChromosome::Y};
    checkUniqueNames(
        chromosomes,
        [](genetics::SexChromosome value) { return chromosomeName(value); });

    const std::array<DeathCause, 4> deathCauses = {
        DeathCause::None, DeathCause::Starved, DeathCause::OldAge,
        DeathCause::Eaten};
    checkUniqueNames(
        deathCauses, [](DeathCause value) { return deathCauseName(value); });

    CHECK(formatClock(125.0) == "02:05");
    CHECK(formatClock(0.0) == "00:00");
}

TEST_CASE("copyLiveTunables copies only live settings") {
    SimConfig destination;

    SimConfig source = destination;
    source.worldSize = 1234.f;
    source.fixedDt = 0.02f;
    source.boundaryMargin = 99.f;
    source.spatialCellSize = 111.f;
    source.initialPlants = 17;
    source.maxPlants = 900;
    source.plantSpreadRadius = 55.f;
    source.plantCrowdRadius = 77.f;
    source.plantMaxNeighbours = 4;
    source.maxEnergy = 150.f;
    source.startEnergy = 95.f;
    source.baseUpkeep = 0.9f;
    source.bodyRadius = 12.f;
    source.wanderTurnMin = 0.7f;
    source.wanderTurnMax = 9.f;
    source.wanderTurnAngle = 1.7f;
    source.senseInterval = 0.8f;
    source.hungerThreshold = 0.4f;
    source.eatReach = 70.f;
    source.preyEnergyValue = 120.f;
    source.predatorMissCooldown = 8.f;
    source.maturityAge = 21.f;
    source.mateEnergyThreshold = 0.9f;
    source.mateReach = 30.f;
    source.gestationTime = 22.f;
    source.litterMin = 2;
    source.litterMax = 4;
    source.pregnancyEnergyCost = 25.f;
    source.matingEnergyCost = 9.f;
    source.postBirthCooldown = 45.f;
    source.maleCooldown = 25.f;
    source.offspringStartEnergy = 55.f;
    source.birthScatter = 35.f;
    source.maxHerbivores = 250;
    source.maxPredators = 90;
    source.rescueFloor = 9;
    source.rescueInterval = 50.f;
    source.statsInterval = 2.f;
    source.initialHerbivores = 44;
    source.herbivoreSpeed = 30.f;
    source.herbivoreMaxAge = 250.f;
    source.initialPredators = 18;
    source.predatorSpeed = 65.f;
    source.predatorMaxAge = 500.f;
    source.predatorUpkeepMultiplier = 0.5f;
    source.predatorStartEnergy = 110.f;
    source.predatorSightMultiplier = 3.f;
    source.predatorPreyRefuge = 30;
    source.genetics.sightMin = 210.f;
    source.genetics.sightMax = 390.f;

    source.genetics.sightMutationRate = 0.31f;
    source.genetics.sightMutationSd = 27.f;
    source.genetics.colourMutationRate = 0.12f;
    source.sightUpkeep = 0.009f;
    source.plantSpawnMin = 3.f;
    source.plantSpawnMax = 17.f;
    source.plantEnergyValue = 45.f;
    source.predatorCatchChance = 0.33f;
    source.enableRescue = false;

    const SimConfig before = destination;
    copyLiveTunables(source, destination);

    CHECK(destination.genetics.sightMutationRate ==
          source.genetics.sightMutationRate);
    CHECK(destination.genetics.sightMutationSd ==
          source.genetics.sightMutationSd);
    CHECK(destination.genetics.colourMutationRate ==
          source.genetics.colourMutationRate);
    CHECK(destination.sightUpkeep == source.sightUpkeep);
    CHECK(destination.plantSpawnMin == source.plantSpawnMin);
    CHECK(destination.plantSpawnMax == source.plantSpawnMax);
    CHECK(destination.plantEnergyValue == source.plantEnergyValue);
    CHECK(destination.predatorCatchChance == source.predatorCatchChance);
    CHECK(destination.enableRescue == source.enableRescue);
    checkNonLiveFieldsUnchanged(before, destination);
}

TEST_CASE("chart data mirrors stats samples and living sights") {
    SimConfig cfg;
    Simulation sim(cfg, 4u);
    sim.populate();

    const int steps = static_cast<int>(100.f / cfg.fixedDt);
    for (int i = 0; i < steps; ++i) {
        sim.step();
    }

    ChartSeries series;
    buildChartSeries(sim.stats(), series);

    const std::vector<StatsSample>& samples = sim.stats().samples();
    REQUIRE(!samples.empty());
    CHECK(series.time.size() == samples.size());
    CHECK(series.plants.size() == samples.size());
    CHECK(series.herbivores.size() == samples.size());
    CHECK(series.predators.size() == samples.size());
    CHECK(series.herbMeanSight.size() == samples.size());
    CHECK(series.predMeanSight.size() == samples.size());
    CHECK(series.herbDarkAlleleFreq.size() == samples.size());
    CHECK(series.predDarkAlleleFreq.size() == samples.size());

    for (std::size_t i = 1; i < series.time.size(); ++i) {
        CHECK(series.time[i] > series.time[i - 1]);
    }
    CHECK(series.herbivores.back() ==
          static_cast<double>(samples.back().herbivores.count));

    const std::vector<float> herbivoreSights =
        livingSights(sim, Kind::Herbivore);
    const std::vector<float> predatorSights =
        livingSights(sim, Kind::Predator);
    CHECK(herbivoreSights.size() ==
          static_cast<std::size_t>(sim.count(Kind::Herbivore)));
    CHECK(predatorSights.size() ==
          static_cast<std::size_t>(sim.count(Kind::Predator)));
    for (float sight : herbivoreSights) {
        CHECK(sight >= 200.f);
        CHECK(sight <= 400.f);
    }
    for (float sight : predatorSights) {
        CHECK(sight >= 200.f);
        CHECK(sight <= 400.f);
    }
    CHECK(livingSights(sim, Kind::Plant).empty());
}
