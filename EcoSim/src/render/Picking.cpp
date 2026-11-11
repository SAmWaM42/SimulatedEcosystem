// Picking implementation: ray/sphere intersection with per-kind radii.
#include "render/Picking.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>

#include <glm/geometric.hpp>

#include "sim/Simulation.h"
#include "world/Terrain.h"

uint32_t pickOrganism(const Ray& ray, const Simulation& sim,
                      const Terrain& terrain) {
    const float bodyRadius = sim.config().bodyRadius;
    float nearest = std::numeric_limits<float>::max();
    uint32_t selected = 0;

    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        if (!organism->alive) {
            continue;
        }

        float radius = 14.f;
        if (organism->kind() == Kind::Herbivore) {
            radius = 16.f;
        } else if (organism->kind() == Kind::Predator) {
            radius = 20.f;
        }

        const glm::vec3 centre(
            organism->position.x,
            terrain.heightAt(organism->position.x, organism->position.y) +
                1.2f * bodyRadius,
            organism->position.y);

        const glm::vec3 offset = centre - ray.origin;
        const float along = glm::dot(offset, ray.direction);
        if (along < 0.f) {
            continue;
        }

        const float distanceSquared =
            glm::dot(offset, offset) - along * along;
        const float radiusSquared = radius * radius;
        if (distanceSquared > radiusSquared) {
            continue;
        }

        const float halfChord =
            std::sqrt(std::max(0.f, radiusSquared - distanceSquared));
        float hit = along - halfChord;
        if (hit < 0.f) {
            hit = along + halfChord;
        }
        if (hit >= 0.f && hit < nearest) {
            nearest = hit;
            selected = organism->id;
        }
    }

    return selected;
}

