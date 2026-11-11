// Punnett-square implementation using the shared complete-dominance mapping.
#include "ui/Punnett.h"

#include <array>

namespace {
// Returns the complete-dominance phenotype for one allele pair.
genetics::ColourPhenotype phenotypeOf(genetics::ColourAllele mother,
                                      genetics::ColourAllele father) {
    if (mother == genetics::ColourAllele::Dark ||
        father == genetics::ColourAllele::Dark) {
        return genetics::ColourPhenotype::Dark;
    }
    if (mother == genetics::ColourAllele::Medium ||
        father == genetics::ColourAllele::Medium) {
        return genetics::ColourPhenotype::Medium;
    }
    return genetics::ColourPhenotype::Light;
}

// True when value equals either parent allele; used to detect mutation.
bool isPresent(double value, const std::array<float, 2>& alleles) {
    return value == alleles[0] || value == alleles[1];
}

// Colour version of isPresent.
bool isPresent(genetics::ColourAllele value,
               const std::array<genetics::ColourAllele, 2>& alleles) {
    return value == alleles[0] || value == alleles[1];
}
}  // namespace

PunnettInfo buildPunnett(const LineageRecord* child,
                         const LineageRecord* mother,
                         const LineageRecord* father) {
    PunnettInfo info{};
    if (child == nullptr || mother == nullptr || father == nullptr) {
        return info;
    }

    info.available = true;
    const genetics::Genome& childGenome = child->genome;
    const genetics::Genome& motherGenome = mother->genome;
    const genetics::Genome& fatherGenome = father->genome;

    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 2; ++column) {
            const genetics::ColourAllele fromMother =
                motherGenome.colour[row];
            const genetics::ColourAllele fromFather =
                fatherGenome.colour[column];
            const bool isChild =
                fromMother == childGenome.colour[0] &&
                fromFather == childGenome.colour[1];
            info.cells[row][column] =
                PunnettCell{fromMother, fromFather,
                            phenotypeOf(fromMother, fromFather), isChild};
        }
    }

    // Slots are fixed by fertilise(): child slot 0 is maternal, slot 1 is
    // paternal. A value outside both parental alleles can only be mutation.
    info.sightMutationMother =
        !isPresent(childGenome.sight[0], motherGenome.sight);
    info.sightMutationFather =
        !isPresent(childGenome.sight[1], fatherGenome.sight);
    info.colourMutationMother =
        !isPresent(childGenome.colour[0], motherGenome.colour);
    info.colourMutationFather =
        !isPresent(childGenome.colour[1], fatherGenome.colour);
    return info;
}
