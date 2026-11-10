// Small value type for the flat square simulation world; clamps positions and
// centralises the containment checks used by organisms and the spatial grid.
#pragma once

#include <algorithm>

#include <glm/vec2.hpp>

// Square world in the x/z plane, with x stored in vec2.x and z in vec2.y.
struct World {
    float size;

    // Returns true when the point is inside the world, edges included.
    bool inside(glm::vec2 p) const {
        return p.x >= 0.f && p.x <= size && p.y >= 0.f && p.y <= size;
    }

    // Clamps a position to the world so movement cannot leave the simulation.
    glm::vec2 clamp(glm::vec2 p) const {
        return {std::clamp(p.x, 0.f, size), std::clamp(p.y, 0.f, size)};
    }

    // Returns the middle of the world, used to steer animals away from edges.
    glm::vec2 centre() const {
        return {size * 0.5f, size * 0.5f};
    }

    // Hook for a later renderer's terrain/water; always walkable in this split.
    bool isWalkable(glm::vec2) const {
        return true;
    }
};
