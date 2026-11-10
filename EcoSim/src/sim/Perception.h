// Detection model shared by animals: sight is both the maximum range and the
// probability of noticing something, falling off linearly with distance.
#pragma once

// Returns the chance to notice a target at distance given the observer's sight.
// Sight is both a range and a probability: certain at contact, zero at the edge.
inline float detectionChance(float distance, float sight) {
    if (sight <= 0.f || distance >= sight) {
        return 0.f;
    }
    return 1.f - distance / sight;
}
