// Implementations of the organism hierarchy: plant propagation, animal upkeep,
// sight-based perception, the behaviour state machine, eating, and movement.
#include "sim/Organism.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include <glm/geometric.hpp>

#include "sim/Perception.h"
#include "sim/Reproduction.h"
#include "sim/Simulation.h"

namespace {
// Defined locally because the standard library does not provide a portable pi.
constexpr float kPi = 3.14159265358979f;
constexpr float kPlantMinSpread = 20.f;
constexpr float kBoundaryJitter = 0.3f;
}  // namespace

Organism::Organism(uint32_t organismId, glm::vec2 startPosition)
    : id(organismId), position(startPosition) {}

Plant::Plant(uint32_t organismId, glm::vec2 startPosition, const SimConfig& cfg,
             Rng& rng)
    : Organism(organismId, startPosition),
      spawnTimer(rng.uniform(0.f, cfg.plantSpawnMax)) {}

Kind Plant::kind() const {
    return Kind::Plant;
}

void Plant::update(float dt, Simulation& sim) {
    if (!alive) {
        return;
    }

    spawnTimer -= dt;
    if (spawnTimer > 0.f) {
        return;
    }

    const SimConfig& cfg = sim.config();
    const float angle = sim.rng().uniform(0.f, 2.f * kPi);
    const float distance =
        sim.rng().uniform(kPlantMinSpread, cfg.plantSpreadRadius);
    const glm::vec2 candidate =
        position + glm::vec2(std::cos(angle), std::sin(angle)) * distance;

    if (sim.world().inside(candidate) &&
        sim.count(Kind::Plant) < cfg.maxPlants) {
        std::vector<Organism*> neighbours;
        sim.grid().queryRadius(candidate, cfg.plantCrowdRadius, neighbours);

        int plantNeighbours = 0;
        for (const Organism* neighbour : neighbours) {
            if (neighbour->kind() == Kind::Plant) {
                ++plantNeighbours;
            }
        }

        if (plantNeighbours < cfg.plantMaxNeighbours) {
            sim.addOrganism(std::make_unique<Plant>(sim.nextId(), candidate,
                                                    cfg, sim.rng()));
        }
    }

    // Re-roll even after a failed attempt so the parent keeps trying later.
    spawnTimer = sim.rng().uniform(cfg.plantSpawnMin, cfg.plantSpawnMax);
}

Animal::Animal(uint32_t organismId, glm::vec2 startPosition,
               const genetics::Genome& sourceGenome, float movementSpeed,
               float lifeSpan, const SimConfig& cfg, Rng& rng)
    : Organism(organismId, startPosition),
      genome(sourceGenome),
      sex(genetics::sexOf(sourceGenome)),
      sight(genetics::sightOf(sourceGenome)),
      energy(cfg.startEnergy),
      speed(movementSpeed),
      maxAge(lifeSpan),
      heading(rng.uniform(0.f, 2.f * kPi)),
      senseTimer(rng.uniform(0.f, cfg.senseInterval)),
      cooldown(0.f),
      wanderTimer(rng.uniform(cfg.wanderTurnMin, cfg.wanderTurnMax)),
      energyCap(cfg.maxEnergy),
      configPtr(&cfg),
      rngPtr(&rng) {}

void Animal::update(float dt, Simulation& sim) {
    if (!alive) {
        return;
    }

    age += dt;

    // Sight costs upkeep here: better detection must repay its energy cost.
    const SimConfig& cfg = sim.config();
    energy -= (cfg.baseUpkeep + cfg.sightUpkeep * sight) *
              upkeepMultiplier(cfg) * dt;

    // Starvation takes precedence if both limits are reached on the same step.
    if (energy <= 0.f) {
        alive = false;
        deathCause = DeathCause::Starved;
        return;
    }
    if (age >= maxAge) {
        alive = false;
        deathCause = DeathCause::OldAge;
        return;
    }

    reproCooldown -= dt;
    if (pregnant()) {
        gestationTimer -= dt;
        if (gestationTimer <= 0.f) {
            giveBirth(sim);
        }
    }

    think(dt, sim);

    position += glm::vec2(std::cos(heading), std::sin(heading)) * speed * dt;
    position = sim.world().clamp(position);
}

State Animal::state() const {
    return currentState;
}

bool Animal::isAdult(const SimConfig& cfg) const {
    return age >= cfg.maturityAge;
}

bool Animal::pregnant() const {
    return !embryos.empty();
}

bool Animal::canMate(const SimConfig& cfg) const {
    return alive && isAdult(cfg) &&
           energy >= cfg.mateEnergyThreshold * cfg.maxEnergy &&
           reproCooldown <= 0.f && !pregnant();
}

bool Animal::seeksMate() const {
    return sex == genetics::Sex::Male;
}

bool Animal::mayHunt(Simulation&) const {
    return true;
}

void Animal::think(float dt, Simulation& sim) {
    const SimConfig& cfg = sim.config();

    senseTimer -= dt;
    cooldown -= dt;

    // Drop targets that no longer exist, have died, or left sight range.
    Organism* food = sim.find(foodId);
    if (food == nullptr || !food->alive ||
        glm::length(food->position - position) > perceptionRange()) {
        foodId = 0;
        food = nullptr;
    }

    Organism* threat = sim.find(threatId);
    if (threat == nullptr || !threat->alive ||
        glm::length(threat->position - position) > perceptionRange()) {
        threatId = 0;
        threat = nullptr;
    }

    Animal* mate = nullptr;
    if (mateId != 0) {
        mate = dynamic_cast<Animal*>(sim.find(mateId));
        if (mate == nullptr || !mate->alive || mate->sex == sex ||
            mate->kind() != kind() ||
            glm::length(mate->position - position) > perceptionRange() ||
            !mate->canMate(cfg)) {
            mateId = 0;
            mate = nullptr;
        }
    }

    const bool hungry = energy < cfg.hungerThreshold * cfg.maxEnergy;
    if (!mayHunt(sim)) {
        foodId = 0;
        food = nullptr;
    }

    if (senseTimer <= 0.f) {
        senseTimer = cfg.senseInterval;

        if (flees() && threatId == 0) {
            threatId = scanFor(Kind::Predator, sim);
        }
        if (hungry && cooldown <= 0.f && foodId == 0 && mayHunt(sim)) {
            foodId = scanFor(foodKind(), sim);
        }
        if (!hungry && seeksMate() && canMate(cfg) && mateId == 0) {
            const auto mateFilter = [&cfg, this](const Organism& candidate) {
                const Animal* animal = dynamic_cast<const Animal*>(&candidate);
                return animal != nullptr && animal->sex != sex &&
                       animal->canMate(cfg);
            };
            mateId = scanFor(kind(), sim, mateFilter);
        }

        // Refresh local pointers after scanning may have set new target ids.
        food = sim.find(foodId);
        threat = sim.find(threatId);
        if (mateId != 0) {
            mate = dynamic_cast<Animal*>(sim.find(mateId));
        }
    }

    // The state machine is the heart of animal behaviour: threats win over
    // hunger, and only a wandering animal uses the random turn/safe-edge logic.
    if (threatId != 0) {
        currentState = State::Flee;
    } else if (hungry && cooldown <= 0.f && foodId != 0) {
        currentState = State::SeekFood;
    } else if (!hungry && seeksMate() && mateId != 0) {
        currentState = State::SeekMate;
    } else {
        currentState = State::Wander;
    }

    switch (currentState) {
    case State::Flee:
        if (threat != nullptr) {
            heading = std::atan2(position.y - threat->position.y,
                                 position.x - threat->position.x);
        }
        break;

    case State::SeekFood:
        if (food != nullptr) {
            const glm::vec2 offset = food->position - position;
            heading = std::atan2(offset.y, offset.x);

            if (glm::length(offset) <= cfg.eatReach) {
                onReachFood(*food, sim);
                foodId = 0;
            }
        }
        break;

    case State::SeekMate:
        if (mate != nullptr) {
            const glm::vec2 offset = mate->position - position;
            heading = std::atan2(offset.y, offset.x);

            if (glm::length(offset) <= cfg.mateReach && canMate(cfg) &&
                mate->canMate(cfg)) {
                if (sex == genetics::Sex::Female) {
                    conceive(*this, *mate, sim);
                } else {
                    conceive(*mate, *this, sim);
                }
                mateId = 0;
            }
        } else {
            mateId = 0;
        }
        break;

    case State::Wander:
        wanderTimer -= dt;
        if (wanderTimer <= 0.f) {
            heading +=
                sim.rng().uniform(-cfg.wanderTurnAngle, cfg.wanderTurnAngle);
            wanderTimer =
                sim.rng().uniform(cfg.wanderTurnMin, cfg.wanderTurnMax);
        }

        // Steer towards the centre near an edge, with a little jitter so animals
        // do not all adopt exactly the same inward heading.
        {
            const World& world = sim.world();
            if (position.x < cfg.boundaryMargin ||
                position.x > world.size - cfg.boundaryMargin ||
                position.y < cfg.boundaryMargin ||
                position.y > world.size - cfg.boundaryMargin) {
                const glm::vec2 toCentre = world.centre() - position;
                heading = std::atan2(toCentre.y, toCentre.x) +
                          sim.rng().uniform(-kBoundaryJitter, kBoundaryJitter);
            }
        }
        break;
    }
}

uint32_t Animal::scanFor(
    Kind kind, Simulation& sim,
    const std::function<bool(const Organism&)>& filter) const {
    std::vector<Organism*> nearby;
    sim.grid().queryRadius(position, perceptionRange(), nearby);

    uint32_t bestId = 0;
    float bestDistance = perceptionRange();

    for (const Organism* candidate : nearby) {
        if (candidate == this || !candidate->alive ||
            candidate->kind() != kind) {
            continue;
        }
        if (filter && !filter(*candidate)) {
            continue;
        }

        const float distance = glm::length(candidate->position - position);
        if (distance >= bestDistance) {
            continue;
        }

        if (sim.rng().chance(detectionChance(distance, perceptionRange()))) {
            bestDistance = distance;
            bestId = candidate->id;
        }
    }

    return bestId;
}

void Animal::giveBirth(Simulation& sim) {
    const SimConfig& cfg = sim.config();
    const int cap = (kind() == Kind::Herbivore) ? cfg.maxHerbivores
                                                : cfg.maxPredators;

    for (const genetics::Genome& embryoGenome : embryos) {
        // Gestation can overlap a boom, so enforce the hard cap again at birth.
        if (sim.count(kind()) >= cap) {
            break;
        }

        const float angle = sim.rng().uniform(0.f, 2.f * kPi);
        const float distance = sim.rng().uniform(0.f, cfg.birthScatter);
        glm::vec2 childPosition =
            position + glm::vec2(std::cos(angle), std::sin(angle)) * distance;
        childPosition = sim.world().clamp(childPosition);

        std::unique_ptr<Animal> child =
            makeChild(sim.nextId(), childPosition, embryoGenome);
        child->motherId = id;
        child->fatherId = embryoFatherId;
        child->generation = embryoGeneration;
        child->energy = cfg.offspringStartEnergy;
        sim.addOrganism(std::move(child));
        sim.noteBirth();
    }

    embryos.clear();
    gestationTimer = 0.f;
    reproCooldown = cfg.postBirthCooldown;
}

void Animal::gainEnergy(float amount) {
    energy = std::min(energyCap, energy + amount);
}

float Animal::upkeepMultiplier(const SimConfig&) const {
    return 1.f;
}

float Animal::perceptionRange() const {
    return sight;
}

Kind Herbivore::kind() const {
    return Kind::Herbivore;
}

Kind Herbivore::foodKind() const {
    return Kind::Plant;
}

bool Herbivore::flees() const {
    return true;
}

void Herbivore::onReachFood(Organism& food, Simulation& sim) {
    food.alive = false;
    food.deathCause = DeathCause::Eaten;
    gainEnergy(sim.config().plantEnergyValue);
}

std::unique_ptr<Animal> Herbivore::makeChild(
    uint32_t childId, glm::vec2 pos, const genetics::Genome& g) const {
    return std::make_unique<Herbivore>(
        childId, pos, g, configPtr->herbivoreSpeed, configPtr->herbivoreMaxAge,
        *configPtr, *rngPtr);
}

Kind Predator::kind() const {
    return Kind::Predator;
}

Kind Predator::foodKind() const {
    return Kind::Herbivore;
}

bool Predator::flees() const {
    return false;
}

void Predator::onReachFood(Organism& food, Simulation& sim) {
    if (sim.rng().chance(sim.config().predatorCatchChance)) {
        food.alive = false;
        food.deathCause = DeathCause::Eaten;
        gainEnergy(sim.config().preyEnergyValue);
    } else {
        cooldown = sim.config().predatorMissCooldown;
        foodId = 0;
    }
}

std::unique_ptr<Animal> Predator::makeChild(
    uint32_t childId, glm::vec2 pos, const genetics::Genome& g) const {
    return std::make_unique<Predator>(
        childId, pos, g, configPtr->predatorSpeed, configPtr->predatorMaxAge,
        *configPtr, *rngPtr);
}

float Predator::upkeepMultiplier(const SimConfig& cfg) const {
    return cfg.predatorUpkeepMultiplier;
}

bool Predator::seeksMate() const {
    return true;
}

float Predator::perceptionRange() const {
    return sight * configPtr->predatorSightMultiplier;
}

bool Predator::mayHunt(Simulation& sim) const {
    return sim.count(Kind::Herbivore) > configPtr->predatorPreyRefuge;
}
