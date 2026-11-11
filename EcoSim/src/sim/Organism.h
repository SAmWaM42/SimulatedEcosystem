// Organism hierarchy for the headless simulation: plants, animals, and the
// herbivore/predator specialisations with behaviour and reproduction.
#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>

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
enum class State { Wander, SeekFood, Flee, SeekMate };

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
    // Saves the current state used by render interpolation.
    virtual void savePrevious();

    uint32_t id;
    glm::vec2 position;
    glm::vec2 prevPosition;  // position at the start of the current step
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

// Shared animal state: upkeep, ageing, perception, decisions, reproduction,
// and movement.
class Animal : public Organism {
public:
    // Caches expressed traits and starts random heading, sense and wander timers.
    Animal(uint32_t organismId, glm::vec2 startPosition,
           const genetics::Genome& sourceGenome, float movementSpeed,
           float lifeSpan, const SimConfig& cfg, Rng& rng);
    // Applies upkeep, ageing, gestation, decisions, and movement for one step.
    void update(float dt, Simulation& sim) override;
    // Saves both position and heading for interpolation.
    void savePrevious() override;

    genetics::Genome genome;
    genetics::Sex sex;
    float sight;
    float energy;
    float speed;
    float maxAge;
    float heading;      // radians; random at birth and changed by the state machine
    float prevHeading;  // heading at the start of the current step
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

    // True once the animal has reached the configured maturity age.
    bool isAdult(const SimConfig& cfg) const;
    // True while the mother carries embryos.
    bool pregnant() const;
    // True when alive, adult, energetic, off cooldown, and not pregnant.
    bool canMate(const SimConfig& cfg) const;

    // Genomes fixed at conception; the mother gives birth when gestation ends.
    std::vector<genetics::Genome> embryos;
    float gestationTimer = 0.f;
    uint32_t embryoFatherId = 0;
    int embryoGeneration = 0;
    // Blocks conception after a birth or a missed male mating opportunity.
    float reproCooldown = 0.f;
    // Id of the female a male is currently courting; 0 means none.
    uint32_t mateId = 0;

protected:
    // Perception/state-machine step: sense, choose a state, then act on it.
    virtual void think(float dt, Simulation& sim);
    // True when this animal actively searches for mates (males by default).
    virtual bool seeksMate() const;
    // Kind this animal searches for when hungry.
    virtual Kind foodKind() const = 0;
    // True when this animal may currently acquire food targets.
    virtual bool mayHunt(Simulation& sim) const;
    // True when this animal tries to escape predators.
    virtual bool flees() const = 0;
    // Applies eating/attack when a food target is within reach.
    virtual void onReachFood(Organism& food, Simulation& sim) = 0;
    // Creates a same-kind child with this animal's configured speed and maxAge.
    virtual std::unique_ptr<Animal> makeChild(uint32_t childId, glm::vec2 pos,
                                              const genetics::Genome& g) const = 0;
    // Turns the current embryos into children and starts the mother cooldown.
    void giveBirth(Simulation& sim);
    // Returns the nearest detected living organism of a kind, optionally
    // filtered before any detection roll (empty filter accepts all).
    uint32_t scanFor(
        Kind kind, Simulation& sim,
        const std::function<bool(const Organism&)>& filter = {}) const;
    // Effective detection radius; predators may amplify their genetic sight.
    virtual float perceptionRange() const;
    // Adds energy and clamps it to the maximum cached at construction.
    void gainEnergy(float amount);
    // Multiplier applied to metabolic upkeep; predators override this.
    virtual float upkeepMultiplier(const SimConfig& cfg) const;

    float wanderTimer;  // seconds until the next random heading change
    float energyCap;    // maxEnergy cached from SimConfig at construction
    const SimConfig* configPtr;  // Simulation-owned; valid for this animal's life
    Rng* rngPtr;                 // shared RNG; valid for this animal's life

private:
    State currentState = State::Wander;
};

// Herbivores seek plants, flee predators, and reproduce with other herbivores.
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
    // Creates a herbivore child with herbivore speed and lifespan.
    std::unique_ptr<Animal> makeChild(uint32_t childId, glm::vec2 pos,
                                      const genetics::Genome& g) const override;
};

// Predators hunt herbivores, never flee, and reproduce with other predators.
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
    // Creates a predator child with predator speed and lifespan.
    std::unique_ptr<Animal> makeChild(uint32_t childId, glm::vec2 pos,
                                      const genetics::Genome& g) const override;
    // Predators use the configured predator-specific upkeep multiplier.
    float upkeepMultiplier(const SimConfig& cfg) const override;
    // Both predator sexes actively search for mates when receptive.
    bool seeksMate() const override;
    // Predators stop hunting when their prey fall below the refuge count.
    bool mayHunt(Simulation& sim) const override;
    // Predators have a hunting sense wider than their raw sight trait.
    float perceptionRange() const override;
};

