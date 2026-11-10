// Organism hierarchy for the headless simulation: plants, animals, and the
// herbivore/predator specialisations with state-machine behaviour.
#pragma once

#include <cstdint>

#include <glm/vec2.hpp>

#include "core/Rng.h"
#include "genetics/Genetics.h"

class Simulation;
struct SimConfig;

// Tags every organism for counting, spatial queries, and interaction rules.
enum class Kind { Plant, Herbivore, Predator };

// Why an organism left the simulation; None until a cause is assigned.
enum class DeathCause { None, Starved, OldAge, Eaten };

// Animal behaviour state chosen by the perception and decision machine.
enum class State { Wander, SeekFood, Flee };

// Abstract base for everything that lives in the world.
class Organism {
public:
    // Stores the identity and starting position shared by all organisms.
    Organism(uint32_t organismId, glm::vec2 startPosition);
    // Virtual destructor so unique_ptr can delete derived objects safely.
    virtual ~Organism() = default;

    // Returns the concrete kind used by counting and spatial queries.
    virtual Kind kind() const = 0;
    // Advances one fixed step; implementations must do nothing when dead.
    virtual void update(float dt, Simulation& sim) = 0;

    uint32_t id;
    glm::vec2 position;
    bool alive = true;  // false marks the organism for removal at the end of a step
    float age = 0.f;
    DeathCause deathCause = DeathCause::None;
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

// Shared animal state: upkeep, ageing, perception, decisions, and movement.
class Animal : public Organism {
public:
    // Caches expressed traits and starts random heading, sense and wander timers.
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
    float heading;  // radians; random at birth and changed by the state machine
    uint32_t motherId = 0;
    uint32_t fatherId = 0;
    int generation = 0;

    // Returns the state chosen during the most recent think().
    State state() const;

    // Id of the organism currently pursued as food; 0 means none.
    uint32_t foodId = 0;
    // Id of the nearest detected predator; 0 means none.
    uint32_t threatId = 0;
    // Seconds until the next perception scan; randomised at birth.
    float senseTimer;
    // Seconds before food scanning is allowed again after a missed attack.
    float cooldown = 0.f;

protected:
    // Perception/state-machine step: sense, choose a state, then act on it.
    virtual void think(float dt, Simulation& sim);
    // Kind this animal searches for when hungry.
    virtual Kind foodKind() const = 0;
    // True when this animal tries to escape predators.
    virtual bool flees() const = 0;
    // Applies eating/attack when a food target is within reach.
    virtual void onReachFood(Organism& food, Simulation& sim) = 0;
    // Returns the nearest detected living organism of a kind, or 0.
    uint32_t scanFor(Kind kind, Simulation& sim) const;
    // Adds energy and clamps it to the maximum cached at construction.
    void gainEnergy(float amount);

    float wanderTimer;  // seconds until the next random heading change
    float energyCap;    // maxEnergy cached from SimConfig at construction

private:
    State currentState = State::Wander;
};

// Herbivores seek plants when hungry and flee from detected predators.
class Herbivore : public Animal {
public:
    // Inherits the Animal constructor so founders can pass genome and stats.
    using Animal::Animal;
    // Returns Kind::Herbivore for counting and interaction rules.
    Kind kind() const override;

protected:
    // Herbivores eat plants.
    Kind foodKind() const override;
    // Herbivores flee from predators.
    bool flees() const override;
    // Consumes the plant and gains its configured energy value.
    void onReachFood(Organism& food, Simulation& sim) override;
};

// Predators hunt herbivores when hungry and never flee.
class Predator : public Animal {
public:
    // Inherits the Animal constructor so founders can pass genome and stats.
    using Animal::Animal;
    // Returns Kind::Predator for counting and interaction rules.
    Kind kind() const override;

protected:
    // Predators eat herbivores.
    Kind foodKind() const override;
    // Predators never flee.
    bool flees() const override;
    // Attempts a catch; a miss starts the predator's cooldown.
    void onReachFood(Organism& food, Simulation& sim) override;
};
