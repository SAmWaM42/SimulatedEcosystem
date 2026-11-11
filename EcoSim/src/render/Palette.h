// Organism colour mapping shared by the renderer and tests; GL-free.
#pragma once

#include <glm/vec3.hpp>

#include "sim/Organism.h"

// Trunk colour used by plant rendering.
constexpr glm::vec3 kTrunkColour{0.40f, 0.28f, 0.15f};

// Returns the visible colour for a kind and colour phenotype.
inline glm::vec3 organismColour(Kind kind, genetics::ColourPhenotype phenotype) {
    if (kind == Kind::Plant) {
        return {0.22f, 0.55f, 0.20f};
    }

    if (kind == Kind::Herbivore) {
        switch (phenotype) {
        case genetics::ColourPhenotype::Dark:
            return {0.32f, 0.20f, 0.12f};
        case genetics::ColourPhenotype::Medium:
            return {0.62f, 0.46f, 0.26f};
        case genetics::ColourPhenotype::Light:
            return {0.92f, 0.86f, 0.70f};
        }
    } else {
        switch (phenotype) {
        case genetics::ColourPhenotype::Dark:
            return {0.17f, 0.16f, 0.18f};
        case genetics::ColourPhenotype::Medium:
            return {0.78f, 0.38f, 0.12f};
        case genetics::ColourPhenotype::Light:
            return {0.75f, 0.72f, 0.82f};
        }
    }

    return {0.22f, 0.55f, 0.20f};
}
