// GL-free picking against generous organism bounding spheres, used by the main
// application to turn a screen click into a selected organism id.
#pragma once

#include <cstdint>

#include "render/Camera.h"

class Simulation;
class Terrain;

// Returns the nearest living organism hit by the ray, or 0 when none is hit.
uint32_t pickOrganism(const Ray& ray, const Simulation& sim,
                      const Terrain& terrain);
