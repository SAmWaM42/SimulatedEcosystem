// Organism hierarchy for the headless simulation: plants, animals, and the
// herbivore/predator specialisations that later splits will extend.
#pragma once

#include <cstdint>

#include <glm/vec2.hpp>

#include "core/Rng.h"
#include "genetics/Genetics.h"

class Simulation;
struct SimConfig;

// Tags every organism for counting, queries, and future interaction rules.
enum class Kind { Plant, Herbivore, Predator };

// Abstract base for everything that lives in the world.
class Organism {
public:
    // Stores the identity and starting position shared by all organisms.
    Organism(uint32_t organismId, glm::vec2 startPosition);
    // Virtual destructor so unique_ptr can delete derived objects safely.
    virtual ~Organism() = default;

    // Returns the concrete kind used by counting and spatial queries.
    virtual Kind kind() const = 0;
    // Advances this organism by one fixed simulation step.
    virtual void update(float dt, Simulation& sim) = 0;

    uint32_t id;
    glm::vec2 position;
    bool alive = true;  // false marks the organism for removal at the end of a step
    float age = 0.f;
};

// Plants have no genes; they periodically try to place a copy nearby.
class Plant : public Organism {
public:
    // Starts the first propagation timer at a random point so plants desynchronise.
    Plant(uint32_t organismId, glm::vec2 startPosition, const SimConfig& cfg,
          Rng& rng);
    // Returns Kind::Plant for queries and population counts.
    Kind kind() const override;
    // Counts down the timer and attempts one nearby propagation when due.
    void update(float dt, Simulation& sim) override;

private:
    float spawnTimer;  // seconds until the next propagation attempt
};

// Shared animal state and behaviour: upkeep, ageing, wandering, and movement.
class Animal : public Organism {
public:
    // Caches expressed traits and starts the animal with random heading and timer.
    Animal(uint32_t organismId, glm::vec2 startPosition,
           const genetics::Genome& sourceGenome, float movementSpeed,
           float lifeSpan, const SimConfig& cfg, Rng& rng);
    // Applies upkeep and ageing, then thinks and moves for one fixed step.
    void update(float dt, Simulation& sim) override;

    genetics::Genome genome;
    genetics::Sex sex;
    float sight;
    float energy;
    float speed;
    float maxAge;
    float heading;  // radians; random at birth and changed by wandering
    uint32_t motherId = 0;
    uint32_t fatherId = 0;
    int generation = 0;

protected:
    // Default behaviour in this split is wandering with boundary avoidance.
    virtual void think(float dt, Simulation& sim);

    float wanderTimer;  // seconds until the next random heading change
};

// Herbivores currently only wander; Split 3 will add food-seeking behaviour.
class Herbivore : public Animal {
public:
    // Inherits the Animal constructor so founders can pass genome and stats.
    using Animal::Animal;
    // Returns Kind::Herbivore for counting and future interaction rules.
    Kind kind() const override;
};

// Predators currently only wander; Split 3 will add hunting behaviour.
class Predator : public Animal {
public:
    // Inherits the Animal constructor so founders can pass genome and stats.
    using Animal::Animal;
    // Returns Kind::Predator for counting and future interaction rules.
    Kind kind() const override;
};
