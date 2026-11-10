// Spatial grid implementation: cell indexing, insertion, and exact-radius
// queries over the organisms inserted for the current simulation step.
#include "sim/SpatialGrid.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

#include <glm/geometric.hpp>

#include "sim/Organism.h"

SpatialGrid::SpatialGrid(float worldSize, float cellSizeInput)
    : cellSize(cellSizeInput),
      cellsPerSide(std::max(
          1, static_cast<int>(std::ceil(worldSize / cellSizeInput)))),
      cells(static_cast<std::size_t>(cellsPerSide * cellsPerSide)) {}

void SpatialGrid::clear() {
    for (std::vector<Organism*>& cell : cells) {
        cell.clear();
    }
}

void SpatialGrid::insert(Organism* o) {
    if (o == nullptr) {
        return;
    }

    const int cx = cellCoordinate(o->position.x);
    const int cy = cellCoordinate(o->position.y);
    cells[static_cast<std::size_t>(cy * cellsPerSide + cx)].push_back(o);
}

void SpatialGrid::queryRadius(glm::vec2 centre, float radius,
                              std::vector<Organism*>& out) const {
    const int minX = cellCoordinate(centre.x - radius);
    const int maxX = cellCoordinate(centre.x + radius);
    const int minY = cellCoordinate(centre.y - radius);
    const int maxY = cellCoordinate(centre.y + radius);
    const float radiusSquared = radius * radius;

    for (int cy = minY; cy <= maxY; ++cy) {
        for (int cx = minX; cx <= maxX; ++cx) {
            const std::vector<Organism*>& cell =
                cells[static_cast<std::size_t>(cy * cellsPerSide + cx)];
            for (Organism* o : cell) {
                const glm::vec2 offset = o->position - centre;
                if (glm::dot(offset, offset) <= radiusSquared) {
                    out.push_back(o);
                }
            }
        }
    }
}

int SpatialGrid::cellCoordinate(float value) const {
    // Clamping before indexing folds every out-of-world point into an edge cell.
    return std::clamp(static_cast<int>(value / cellSize), 0, cellsPerSide - 1);
}
