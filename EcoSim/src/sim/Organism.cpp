// Implementations of the organism hierarchy: plant propagation, animal upkeep,
// wandering, and movement. All behaviour is headless and grid-aware.
#include "sim/Organism.h"

#include <cmath>
#include <memory>
#include <vector>

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
      wanderTimer(rng.uniform(cfg.wanderTurnMin, cfg.wanderTurnMax)) {}

void Animal::update(float dt, Simulation& sim) {
    age += dt;

    // Sight costs upkeep here: better detection must repay its energy cost.
    const SimConfig& cfg = sim.config();
    energy -= (cfg.baseUpkeep + cfg.sightUpkeep * sight) * dt;

    if (energy <= 0.f || age >= maxAge) {
        alive = false;
        return;
    }

    think(dt, sim);

    position += glm::vec2(std::cos(heading), std::sin(heading)) * speed * dt;
    position = sim.world().clamp(position);
}

void Animal::think(float dt, Simulation& sim) {
    const SimConfig& cfg = sim.config();
    wanderTimer -= dt;

    if (wanderTimer <= 0.f) {
        heading += sim.rng().uniform(-cfg.wanderTurnAngle, cfg.wanderTurnAngle);
        wanderTimer = sim.rng().uniform(cfg.wanderTurnMin, cfg.wanderTurnMax);
    }

    // Steer towards the centre near an edge, with a little jitter so animals
    // do not all adopt exactly the same inward heading.
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

Kind Herbivore::kind() const {
    return Kind::Herbivore;
}

Kind Predator::kind() const {
    return Kind::Predator;
}
