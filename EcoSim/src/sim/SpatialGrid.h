// Uniform grid rebuilt every step to answer "who is near me?" without scanning
// the whole population; it stores non-owning pointers to organisms.
#pragma once

#include <vector>

#include <glm/vec2.hpp>

class Organism;

// Fixed-size square-cell grid over the world; queries use exact distances.
class SpatialGrid {
public:
    // Allocates one empty cell list per grid square covering the world.
    SpatialGrid(float worldSize, float cellSizeInput);

    // Empties every cell while keeping the allocated cell structures.
    void clear();

    // Inserts an organism into the cell containing its current position.
    void insert(Organism* o);

    // Appends every inserted organism within radius, using an exact distance
    // test after visiting only the cells overlapping the query square.
    void queryRadius(glm::vec2 centre, float radius,
                     std::vector<Organism*>& out) const;

private:
    // Maps a coordinate to a clamped cell index, including out-of-world points.
    int cellCoordinate(float value) const;

    float cellSize;
    int cellsPerSide;
    std::vector<std::vector<Organism*>> cells;
};
