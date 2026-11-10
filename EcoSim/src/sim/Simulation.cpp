// Simulation implementation: founder creation, fixed-step population updates,
// and the small amount of state shared with organisms.
#include "sim/Simulation.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>

#include <glm/vec2.hpp>

namespace {
// Picks a uniformly random point anywhere inside the square world.
glm::vec2 randomPosition(const World& world, Rng& rng) {
    return {rng.uniform(0.f, world.size), rng.uniform(0.f, world.size)};
}
}  // namespace

Simulation::Simulation(const SimConfig& cfg, uint32_t seed)
    : settings(cfg),
      random(seed),
      worldData{cfg.worldSize},
      spatialGrid(cfg.worldSize, cfg.spatialCellSize) {}

void Simulation::populate() {
    for (int i = 0; i < settings.initialPlants; ++i) {
        addOrganism(std::make_unique<Plant>(nextId(),
                                            randomPosition(worldData, random),
                                            settings, random));
    }

    for (int i = 0; i < settings.initialHerbivores; ++i) {
        const genetics::Sex sex =
            (i % 2 == 0) ? genetics::Sex::Female : genetics::Sex::Male;
        const glm::vec2 position = randomPosition(worldData, random);
        const genetics::Genome genome =
            genetics::randomGenome(sex, settings.genetics, random);
        addOrganism(std::make_unique<Herbivore>(
            nextId(), position, genome, settings.herbivoreSpeed,
            settings.herbivoreMaxAge, settings, random));
    }

    for (int i = 0; i < settings.initialPredators; ++i) {
        const genetics::Sex sex =
            (i % 2 == 0) ? genetics::Sex::Female : genetics::Sex::Male;
        const glm::vec2 position = randomPosition(worldData, random);
        const genetics::Genome genome =
            genetics::randomGenome(sex, settings.genetics, random);
        addOrganism(std::make_unique<Predator>(
            nextId(), position, genome, settings.predatorSpeed,
            settings.predatorMaxAge, settings, random));
    }

    flushPending();
}

void Simulation::step() {
    spatialGrid.clear();
    byId.clear();
    for (const std::unique_ptr<Organism>& organism : population) {
        if (organism->alive) {
            spatialGrid.insert(organism.get());
            byId.emplace(organism->id, organism.get());
        }
    }

    // The population vector is not resized during updates; births go to pending.
    const std::size_t populationSize = population.size();
    for (std::size_t i = 0; i < populationSize; ++i) {
        if (population[i]->alive) {
            population[i]->update(settings.fixedDt, *this);
        }
    }

    // Record every removal before destroying the objects; also drop their ids
    // from the lookup so find() never sees a dangling pointer between steps.
    for (const std::unique_ptr<Organism>& organism : population) {
        if (!organism->alive) {
            const int kindIndex = static_cast<int>(organism->kind());
            const int causeIndex = static_cast<int>(organism->deathCause);
            ++deathCounts[kindIndex][causeIndex];
            byId.erase(organism->id);
        }
    }

    population.erase(
        std::remove_if(population.begin(), population.end(),
                       [](const std::unique_ptr<Organism>& organism) {
                           return !organism->alive;
                       }),
        population.end());

    flushPending();
    simTime += static_cast<double>(settings.fixedDt);
}

void Simulation::addOrganism(std::unique_ptr<Organism> o) {
    if (o != nullptr) {
        pending.push_back(std::move(o));
    }
}

uint32_t Simulation::nextId() {
    return nextIdValue++;
}

const SimConfig& Simulation::config() const {
    return settings;
}

Rng& Simulation::rng() {
    return random;
}

const World& Simulation::world() const {
    return worldData;
}

SpatialGrid& Simulation::grid() {
    return spatialGrid;
}

double Simulation::time() const {
    return simTime;
}

const std::vector<std::unique_ptr<Organism>>& Simulation::organisms() const {
    return population;
}

int Simulation::count(Kind k) const {
    const auto countIn = [k](const std::vector<std::unique_ptr<Organism>>& list) {
        int total = 0;
        for (const std::unique_ptr<Organism>& organism : list) {
            if (organism->alive && organism->kind() == k) {
                ++total;
            }
        }
        return total;
    };

    return countIn(population) + countIn(pending);
}

float Simulation::meanSight(Kind k) const {
    if (k == Kind::Plant) {
        return 0.f;
    }

    double sightSum = 0.0;
    int animalCount = 0;
    const auto accumulate = [&](const std::vector<std::unique_ptr<Organism>>& list) {
        for (const std::unique_ptr<Organism>& organism : list) {
            if (!organism->alive || organism->kind() != k) {
                continue;
            }
            const Animal* animal = dynamic_cast<const Animal*>(organism.get());
            if (animal != nullptr) {
                sightSum += static_cast<double>(animal->sight);
                ++animalCount;
            }
        }
    };

    accumulate(population);
    accumulate(pending);

    if (animalCount == 0) {
        return 0.f;
    }
    return static_cast<float>(sightSum / static_cast<double>(animalCount));
}

Organism* Simulation::find(uint32_t id) const {
    const auto entry = byId.find(id);
    if (entry == byId.end() || entry->second == nullptr ||
        !entry->second->alive) {
        return nullptr;
    }
    return entry->second;
}

int Simulation::deathCount(Kind kind, DeathCause cause) const {
    return deathCounts[static_cast<int>(kind)][static_cast<int>(cause)];
}

void Simulation::flushPending() {
    for (std::unique_ptr<Organism>& organism : pending) {
        byId[organism->id] = organism.get();
        population.push_back(std::move(organism));
    }
    pending.clear();
}
