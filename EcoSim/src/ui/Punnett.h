// GL-free Punnett-square data for a recorded child and its two recorded
// parents; also flags alleles that mutation changed during gamete formation.
#pragma once

#include "genetics/Genetics.h"
#include "sim/Lineage.h"

// One mother-allele x father-allele combination in the square.
struct PunnettCell {
    genetics::ColourAllele fromMother;
    genetics::ColourAllele fromFather;
    genetics::ColourPhenotype phenotype;
    bool isChild;
};

// Ready-to-display Punnett data; unavailable when any record is missing.
struct PunnettInfo {
    bool available = false;
    PunnettCell cells[2][2];
    bool sightMutationMother = false;
    bool sightMutationFather = false;
    bool colourMutationMother = false;
    bool colourMutationFather = false;
};

// Builds the square and mutation flags from retained family records.
PunnettInfo buildPunnett(const LineageRecord* child,
                         const LineageRecord* mother,
                         const LineageRecord* father);
