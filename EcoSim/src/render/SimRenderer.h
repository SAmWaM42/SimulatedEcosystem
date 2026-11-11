// Animated rendering of plants, herbivores, and predators with interpolation,
// walk cycles, selection highlighting, and short death effects.
#pragma once

#include <cstdint>
#include <unordered_map>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "render/Mesh.h"
#include "sim/Organism.h"

class Shader;
class Simulation;
class Terrain;

// Draws all living organisms plus recently deceased visual snapshots.
class SimRenderer {
public:
    // Builds the primitive meshes used for every organism part.
    SimRenderer();
    // Draws the current population using shared uniforms already set on lit.
    void draw(const Simulation& sim, const Terrain& terrain, Shader& lit,
              float alpha, float frameDt, uint32_t selectedId);
    // Forgets snapshots and active death effects.
    void reset();
    // Number of primitive draw calls issued by the most recent draw().
    int drawCalls() const;

private:
    struct Snapshot {
        uint32_t id;
        Kind kind;
        glm::vec2 position;
        float heading;
        glm::vec3 colour;
        float scale;
    };

    struct DeathEffect {
        uint32_t id;
        Kind kind;
        glm::vec2 position;
        float heading;
        glm::vec3 colour;
        float scale;
        float elapsed = 0.f;
    };

    // Sets per-part uniforms and issues one indexed draw.
    void drawMesh(const Mesh& mesh, const glm::mat4& model,
                  const glm::vec3& colour, const glm::vec3& emissive);
    // Draws a two-part plant at a world position.
    void drawPlant(glm::vec2 position, float groundHeight, float heading,
                   float overallScale, float sway, const glm::vec3& emissive,
                   float bodyRadius);
    // Draws an animal with body, head, legs, tail, and optional crest/snout.
    void drawAnimal(Kind kind, glm::vec2 position, float groundHeight,
                    float heading, const glm::vec3& colour,
                    float overallScale, bool male, float phase, float roll,
                    const glm::vec3& emissive, float bodyRadius);

    Mesh cubeMesh;
    Mesh sphereMesh;
    Mesh cylinderMesh;
    Mesh coneMesh;
    Shader* activeShader = nullptr;
    int currentDrawCalls = 0;
    int lastDrawCalls = 0;
    std::unordered_map<uint32_t, Snapshot> previous;
    std::unordered_map<uint32_t, DeathEffect> deathEffects;
};
