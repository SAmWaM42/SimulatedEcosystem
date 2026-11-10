// Simulation implementation: founder creation, fixed-step population updates,
// and the small amount of state shared with organisms.
#include "sim/Simulation.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

#include <glm/vec2.hpp>

namespace {
// Picks a uniformly random point anywhere inside the square world.
glm::vec2 randomPosition(const World& world, Rng& rng) {
    return {rng.uniform(0.f, world.size), rng.uniform(0.f, world.size)};
}

// Produces a small square offset used to keep paired founders close together.
glm::vec2 randomJitter(Rng& rng, float radius) {
    return {rng.uniform(-radius, radius), rng.uniform(-radius, radius)};
}
}  // namespace

Simulation::Simulation(const SimConfig& cfg, uint32_t seed)
    : settings(cfg),
      random(seed),
      worldData{cfg.worldSize},
      spatialGrid(cfg.worldSize, cfg.spatialCellSize) {
    nextRescueTime = static_cast<double>(settings.rescueInterval);
    nextStatsTime = static_cast<double>(settings.statsInterval);
}

void Simulation::populate() {
    for (int i = 0; i < settings.initialPlants; ++i) {
        addOrganism(std::make_unique<Plant>(nextId(),
                                            randomPosition(worldData, random),
                                            settings, random));
    }

    std::vector<glm::vec2> herbivorePositions;
    herbivorePositions.reserve(
        static_cast<std::size_t>(settings.initialHerbivores));
    for (int i = 0; i < settings.initialHerbivores; ++i) {
        const genetics::Sex sex =
            (i % 2 == 0) ? genetics::Sex::Female : genetics::Sex::Male;
        const glm::vec2 position = randomPosition(worldData, random);
        herbivorePositions.push_back(position);
        const genetics::Genome genome =
            genetics::randomGenome(sex, settings.genetics, random);
        addOrganism(std::make_unique<Herbivore>(
            nextId(), position, genome, settings.herbivoreSpeed,
            settings.herbivoreMaxAge, settings, random));
    }

    // Predator founders start in mixed-sex pairs near herbivores: random
    // predators in a huge world otherwise die before ever meeting a mate.
    const auto createPredator = [this](glm::vec2 position,
                                       genetics::Sex sex) {
        const genetics::Genome genome =
            genetics::randomGenome(sex, settings.genetics, random);
        std::unique_ptr<Predator> predator = std::make_unique<Predator>(
            nextId(), position, genome, settings.predatorSpeed,
            settings.predatorMaxAge, settings, random);
        predator->energy = settings.predatorStartEnergy;
        // Paired founders start mature so they can seed a population before
        // random wandering and starvation separate them.
        predator->age = settings.maturityAge;
        addOrganism(std::move(predator));
    };

    constexpr float kPredatorPairRadius = 4.f;
    for (int pair = 0; pair * 2 + 1 < settings.initialPredators; ++pair) {
        glm::vec2 pairCentre = randomPosition(worldData, random);
        if (!herbivorePositions.empty()) {
            const int index = random.uniformInt(
                0, static_cast<int>(herbivorePositions.size()) - 1);
            pairCentre = herbivorePositions[static_cast<std::size_t>(index)];
        }
        createPredator(
            worldData.clamp(pairCentre + randomJitter(random, kPredatorPairRadius)),
            genetics::Sex::Female);
        createPredator(
            worldData.clamp(pairCentre + randomJitter(random, kPredatorPairRadius)),
            genetics::Sex::Male);
    }
    if (settings.initialPredators % 2 != 0) {
        createPredator(randomPosition(worldData, random),
                       genetics::Sex::Female);
    }

    flushPending();
    statsManager.sample(*this);
    nextStatsTime = simTime + static_cast<double>(settings.statsInterval);
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
            Animal* animal = dynamic_cast<Animal*>(organism.get());
            if (animal != nullptr) {
                lineageTracker.recordDeath(*animal, simTime);
            }
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

    const double halfStep = static_cast<double>(settings.fixedDt) * 0.5;
    while (settings.enableRescue && settings.rescueInterval > 0.f &&
           simTime + halfStep >= nextRescueTime) {
        spawnRescueImmigrants();
        flushPending();
        nextRescueTime += static_cast<double>(settings.rescueInterval);
    }

    while (settings.statsInterval > 0.f &&
           simTime + halfStep >= nextStatsTime) {
        statsManager.sample(*this);
        nextStatsTime += static_cast<double>(settings.statsInterval);
    }
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

LineageTracker& Simulation::lineage() {
    return lineageTracker;
}

const LineageTracker& Simulation::lineage() const {
    return lineageTracker;
}

StatsManager& Simulation::stats() {
    return statsManager;
}

const StatsManager& Simulation::stats() const {
    return statsManager;
}

void Simulation::noteBirth() {
    ++birthCounter;
}

int Simulation::birthCount() const {
    return birthCounter;
}

int Simulation::immigrantCount() const {
    return immigrantCounter;
}

void Simulation::flushPending() {
    const std::size_t firstNew = population.size();
    for (std::unique_ptr<Organism>& organism : pending) {
        byId[organism->id] = organism.get();
        population.push_back(std::move(organism));
    }
    pending.clear();

    for (std::size_t i = firstNew; i < population.size(); ++i) {
        Animal* animal = dynamic_cast<Animal*>(population[i].get());
        if (animal != nullptr) {
            lineageTracker.recordBirth(*animal, simTime);
        }
    }
}

void Simulation::spawnRescueImmigrants() {
    // Demo safety net: keeps small animal populations from vanishing entirely;
    // one immigrant per underpopulated kind, sexed to balance the survivors.
    const auto spawnOne = [this](Kind kind) {
        if (count(kind) >= settings.rescueFloor) {
            return;
        }

        int females = 0;
        int males = 0;
        for (const std::unique_ptr<Organism>& organism : population) {
            if (!organism->alive || organism->kind() != kind) {
                continue;
            }
            const Animal* animal =
                dynamic_cast<const Animal*>(organism.get());
            if (animal != nullptr) {
                if (animal->sex == genetics::Sex::Female) {
                    ++females;
                } else {
                    ++males;
                }
            }
        }

        const genetics::Sex sex =
            (females <= males) ? genetics::Sex::Female : genetics::Sex::Male;
        const glm::vec2 position = randomPosition(worldData, random);
        const genetics::Genome genome =
            genetics::randomGenome(sex, settings.genetics, random);

        if (kind == Kind::Herbivore) {
            addOrganism(std::make_unique<Herbivore>(
                nextId(), position, genome, settings.herbivoreSpeed,
                settings.herbivoreMaxAge, settings, random));
        } else {
            std::unique_ptr<Predator> predator = std::make_unique<Predator>(
                nextId(), position, genome, settings.predatorSpeed,
                settings.predatorMaxAge, settings, random);
            predator->energy = settings.predatorStartEnergy;
            addOrganism(std::move(predator));
        }
        ++immigrantCounter;
    };

    spawnOne(Kind::Herbivore);
    spawnOne(Kind::Predator);
}
