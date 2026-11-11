// SimRenderer implementation: interpolated placement, primitive part assembly,
// animation, selection emissive, and death snapshots.
#include "render/SimRenderer.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "render/MeshFactory.h"
#include "render/Palette.h"
#include "render/Shader.h"
#include "sim/Simulation.h"
#include "world/Terrain.h"

namespace {
constexpr float kPi = 3.14159265358979f;

// Shortest signed angular difference, used to interpolate headings.
float lerpAngle(float from, float to, float alpha) {
    const float delta = std::remainder(to - from, 2.f * kPi);
    return from + delta * alpha;
}

// A small deterministic hash so plants keep a stable size variation.
float hash01(uint32_t id) {
    uint32_t h = id * 747796405u + 2891336453u;
    h ^= h >> 15;
    h *= 2246822519u;
    h ^= h >> 13;
    return static_cast<float>(h & 0x00ffffffu) / 16777216.f;
}

// Smoothstep without depending on GLSL/GLM extensions.
float smoothstep(float edge0, float edge1, float value) {
    const float t = std::clamp((value - edge0) / (edge1 - edge0), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

// Walk cycle frequency for each behaviour state.
float gaitForState(State state) {
    switch (state) {
    case State::Wander:
        return 1.f;
    case State::SeekFood:
    case State::SeekMate:
        return 1.3f;
    case State::Flee:
        return 2.f;
    }
    return 1.f;
}
}  // namespace

SimRenderer::SimRenderer()
    : cubeMesh(makeCube()),
      sphereMesh(makeSphere()),
      cylinderMesh(makeCylinder()),
      coneMesh(makeCone()) {}

void SimRenderer::reset() {
    previous.clear();
    deathEffects.clear();
    currentDrawCalls = 0;
    lastDrawCalls = 0;
}

int SimRenderer::drawCalls() const {
    return lastDrawCalls;
}

void SimRenderer::drawMesh(const Mesh& mesh, const glm::mat4& model,
                           const glm::vec3& colour,
                           const glm::vec3& emissive) {
    activeShader->setMat4("uModel", model);
    activeShader->setMat3("uNormalMatrix",
                          glm::inverseTranspose(glm::mat3(model)));
    activeShader->setVec3("uColor", colour);
    activeShader->setVec3("uEmissive", emissive);
    mesh.draw();
    ++currentDrawCalls;
}

void SimRenderer::drawPlant(glm::vec2 position, float groundHeight,
                            float heading, float overallScale, float sway,
                            const glm::vec3& emissive, float bodyRadius) {
    // Model forward is +X; plants do not use heading, but keeping the same
    // transform convention makes the snapshot/death path consistent.
    const glm::mat4 base =
        glm::translate(glm::mat4(1.f),
                       glm::vec3(position.x, groundHeight, position.y)) *
        glm::rotate(glm::mat4(1.f), -heading, glm::vec3(0.f, 1.f, 0.f)) *
        glm::rotate(glm::mat4(1.f), sway, glm::vec3(0.f, 0.f, 1.f)) *
        glm::scale(glm::mat4(1.f), glm::vec3(overallScale));

    const glm::mat4 trunk =
        glm::translate(glm::mat4(1.f),
                       glm::vec3(0.f, 0.7f * bodyRadius, 0.f)) *
        glm::scale(glm::mat4(1.f),
                   glm::vec3(0.25f * bodyRadius, 1.4f * bodyRadius,
                             0.25f * bodyRadius));
    drawMesh(cylinderMesh, base * trunk, kTrunkColour, emissive);

    const glm::mat4 canopy =
        glm::translate(glm::mat4(1.f),
                       glm::vec3(0.f, 1.95f * bodyRadius, 0.f)) *
        glm::scale(glm::mat4(1.f),
                   glm::vec3(2.2f * bodyRadius));
    const glm::vec3 foliage =
        organismColour(Kind::Plant, genetics::ColourPhenotype::Light);
    drawMesh(sphereMesh, base * canopy, foliage, emissive);
}

void SimRenderer::drawAnimal(Kind kind, glm::vec2 position, float groundHeight,
                             float heading, const glm::vec3& colour,
                             float overallScale, bool male, float phase,
                             float roll, const glm::vec3& emissive,
                             float bodyRadius) {
    // Rotating local +X by -heading about Y points it along the simulation
    // heading (cos, 0, sin); the extra X roll is only used for death.
    const glm::mat4 base =
        glm::translate(glm::mat4(1.f),
                       glm::vec3(position.x, groundHeight, position.y)) *
        glm::rotate(glm::mat4(1.f), -heading, glm::vec3(0.f, 1.f, 0.f)) *
        glm::rotate(glm::mat4(1.f), roll, glm::vec3(1.f, 0.f, 0.f)) *
        glm::scale(glm::mat4(1.f), glm::vec3(overallScale));

    const float radius = bodyRadius;
    const bool predator = (kind == Kind::Predator);
    const float bodyLength = (predator ? 3.4f : 2.6f) * radius;
    const float bodyHeight = 1.5f * radius;
    const float bodyWidth = 1.5f * radius;
    const float bodyCentreY = (predator ? 1.6f : 1.4f) * radius;
    const float bob = 0.08f * radius * std::sin(2.f * phase);
    const glm::vec3 legColour = colour * 0.7f;

    const glm::mat4 body =
        glm::translate(glm::mat4(1.f),
                       glm::vec3(0.f, bodyCentreY + bob, 0.f)) *
        glm::scale(glm::mat4(1.f),
                   glm::vec3(bodyLength, bodyHeight, bodyWidth));
    drawMesh(sphereMesh, base * body, colour, emissive);

    const float hipY = bodyCentreY - 0.45f * radius;
    const float legLength = hipY;
    const float legWidth = 0.18f * radius;
    const glm::vec3 legHips[4] = {
        {bodyLength * 0.30f, hipY, bodyWidth * 0.35f},
        {bodyLength * 0.30f, hipY, -bodyWidth * 0.35f},
        {-bodyLength * 0.30f, hipY, bodyWidth * 0.35f},
        {-bodyLength * 0.30f, hipY, -bodyWidth * 0.35f}};
    const float legOffsets[4] = {0.f, kPi, kPi, 0.f};
    for (int i = 0; i < 4; ++i) {
        const float swing = 0.6f * std::sin(phase + legOffsets[i]);
        const glm::mat4 leg =
            glm::translate(glm::mat4(1.f), legHips[i]) *
            glm::rotate(glm::mat4(1.f), swing, glm::vec3(0.f, 0.f, 1.f)) *
            glm::translate(glm::mat4(1.f),
                           glm::vec3(0.f, -0.5f * legLength, 0.f)) *
            glm::scale(glm::mat4(1.f),
                       glm::vec3(legWidth, legLength, legWidth));
        drawMesh(cylinderMesh, base * leg, legColour, emissive);
    }

    const float headX = bodyLength * 0.32f;
    const float headY = bodyCentreY + 0.55f * radius + bob;
    const glm::mat4 head =
        glm::translate(glm::mat4(1.f), glm::vec3(headX, headY, 0.f)) *
        glm::scale(glm::mat4(1.f), glm::vec3(1.3f * radius));
    drawMesh(sphereMesh, base * head, colour, emissive);

    if (predator) {
        const glm::mat4 snout =
            glm::translate(glm::mat4(1.f),
                           glm::vec3(headX + 0.55f * radius, headY, 0.f)) *
            glm::rotate(glm::mat4(1.f), -0.5f * kPi,
                        glm::vec3(0.f, 0.f, 1.f)) *
            glm::scale(glm::mat4(1.f),
                       glm::vec3(0.45f * radius, 0.9f * radius,
                                 0.45f * radius));
        drawMesh(coneMesh, base * snout, colour * 0.85f, emissive);

        const glm::mat4 tail =
            glm::translate(glm::mat4(1.f),
                           glm::vec3(-bodyLength * 0.52f,
                                     bodyCentreY + 0.1f * radius + bob, 0.f)) *
            glm::rotate(glm::mat4(1.f), 0.5f * kPi,
                        glm::vec3(0.f, 0.f, 1.f)) *
            glm::scale(glm::mat4(1.f),
                       glm::vec3(0.5f * radius, 1.2f * radius,
                                 0.5f * radius));
        drawMesh(coneMesh, base * tail, colour, emissive);
    } else {
        const glm::mat4 tail =
            glm::translate(glm::mat4(1.f),
                           glm::vec3(-bodyLength * 0.52f,
                                     bodyCentreY + 0.15f * radius + bob, 0.f)) *
            glm::scale(glm::mat4(1.f),
                       glm::vec3(0.5f * radius, 0.18f * radius,
                                 0.18f * radius));
        drawMesh(cubeMesh, base * tail, colour, emissive);
    }

    if (male) {
        const glm::mat4 crest =
            glm::translate(glm::mat4(1.f),
                           glm::vec3(headX, headY + 0.95f * radius, 0.f)) *
            glm::scale(glm::mat4(1.f),
                       glm::vec3(0.3f * radius, 0.6f * radius,
                                 0.3f * radius));
        drawMesh(coneMesh, base * crest, colour, emissive);
    }
}

void SimRenderer::draw(const Simulation& sim, const Terrain& terrain,
                       Shader& lit, float alpha, float frameDt,
                       uint32_t selectedId) {
    activeShader = &lit;
    currentDrawCalls = 0;

    // Any snapshot id that is no longer alive becomes a one-second effect.
    for (const auto& entry : previous) {
        if (sim.find(entry.first) == nullptr) {
            const Snapshot& snap = entry.second;
            DeathEffect effect;
            effect.id = snap.id;
            effect.kind = snap.kind;
            effect.position = snap.position;
            effect.heading = snap.heading;
            effect.colour = snap.colour;
            effect.scale = snap.scale;
            deathEffects.emplace(effect.id, effect);
        }
    }

    const float bodyRadius = sim.config().bodyRadius;
    const float time = static_cast<float>(sim.time());
    std::unordered_map<uint32_t, Snapshot> next;

    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        if (!organism->alive) {
            continue;
        }

        const glm::vec2 interpolated =
            organism->prevPosition +
            (organism->position - organism->prevPosition) * alpha;
        const float ground = terrain.heightAt(interpolated.x, interpolated.y);
        const bool selected = (organism->id == selectedId);
        const glm::vec3 emissive =
            selected ? glm::vec3(0.25f, 0.25f, 0.05f) : glm::vec3(0.f);

        if (organism->kind() == Kind::Plant) {
            const float sizeVariation = 0.85f + 0.3f * hash01(organism->id);
            const float grow = smoothstep(0.f, 4.f, organism->age);
            const float overall = sizeVariation * grow;
            const float sway = 0.03f * std::sin(time + organism->id);
            const glm::vec3 colour =
                organismColour(Kind::Plant, genetics::ColourPhenotype::Light);
            drawPlant(interpolated, ground, 0.f, overall, sway, emissive,
                      bodyRadius);
            next.emplace(organism->id,
                         Snapshot{organism->id, Kind::Plant, interpolated, 0.f,
                                  colour, overall});
            continue;
        }

        const Animal* animal = dynamic_cast<const Animal*>(organism.get());
        if (animal == nullptr) {
            continue;
        }

        const float heading =
            lerpAngle(animal->prevHeading, animal->heading, alpha);
        const glm::vec3 colour =
            organismColour(animal->kind(),
                           genetics::colourOf(animal->genome));
        const bool male = (animal->sex == genetics::Sex::Male);
        const float ageFraction =
            std::clamp(animal->age / sim.config().maturityAge, 0.f, 1.f);
        const float ageScale = 0.5f + 0.5f * ageFraction;
        const float popIn = smoothstep(0.f, 1.f, animal->age);
        const float overall = ageScale * popIn * (male ? 1.1f : 1.f);
        const float phase =
            8.f * time * gaitForState(animal->state()) + 0.9f * animal->id;
        drawAnimal(animal->kind(), interpolated, ground, heading, colour,
                   overall, male, phase, 0.f, emissive, bodyRadius);
        next.emplace(organism->id,
                     Snapshot{organism->id, animal->kind(), interpolated,
                              heading, colour, overall});
    }

    for (auto it = deathEffects.begin(); it != deathEffects.end();) {
        DeathEffect& effect = it->second;
        effect.elapsed += frameDt;
        if (effect.elapsed >= 1.f) {
            it = deathEffects.erase(it);
            continue;
        }

        const float shrink = 1.f - std::clamp(effect.elapsed, 0.f, 1.f);
        const float ground =
            terrain.heightAt(effect.position.x, effect.position.y);
        if (effect.kind == Kind::Plant) {
            drawPlant(effect.position, ground, effect.heading,
                      effect.scale * shrink, 0.f, glm::vec3(0.f),
                      bodyRadius);
        } else {
            const float roll =
                std::clamp(effect.elapsed, 0.f, 1.f) * 0.5f * kPi;
            drawAnimal(effect.kind, effect.position, ground, effect.heading,
                       effect.colour, effect.scale * shrink, false,
                       0.9f * effect.id, roll, glm::vec3(0.f), bodyRadius);
        }
        ++it;
    }

    previous = std::move(next);
    lastDrawCalls = currentDrawCalls;
    activeShader = nullptr;
}

