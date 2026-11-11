// GL-free display names for simulation/genetics enums and clock formatting;
// shared by the ImGui panels and unit tests.
#pragma once

#include <string>

#include "genetics/Genetics.h"
#include "sim/Organism.h"

// Returns a human-readable organism kind.
std::string kindName(Kind kind);
// Returns a human-readable behaviour state.
std::string stateName(State state);
// Returns a human-readable visible colour phenotype.
std::string phenotypeName(genetics::ColourPhenotype phenotype);
// Returns a human-readable colour allele.
std::string alleleName(genetics::ColourAllele allele);
// Returns "X" or "Y".
std::string chromosomeName(genetics::SexChromosome chromosome);
// Returns a human-readable death cause.
std::string deathCauseName(DeathCause cause);
// Formats simulated seconds as "MM:SS"; values below zero are treated as zero.
std::string formatClock(double seconds);
