// Genetics core for animals: genomes, gametes, fertilisation, and trait
// expression. It is independent of graphics so it can be tested headless.
#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "core/Rng.h"

namespace genetics {

// Biological sex of an animal.
enum class Sex { Female, Male };

// Sex chromosome carried by a gamete; a Y anywhere makes the animal male.
enum class SexChromosome : uint8_t { X, Y };

// Colour allele ordered by dominance: lower numeric value is more dominant.
enum class ColourAllele : uint8_t { Dark = 0, Medium = 1, Light = 2 };

// Visible colour after complete dominance is applied to the allele pair.
enum class ColourPhenotype : uint8_t { Dark, Medium, Light };

// Tunable ranges and mutation rates for the genetics model.
struct GeneticsConfig {
    float sightMin = 200.f;
    float sightMax = 400.f;
    float sightMutationRate = 0.05f;
    float sightMutationSd = 10.f;
    float colourMutationRate = 0.01f;
};

// Plain copyable genome: two alleles for each gene and the sex chromosomes.
struct Genome {
    std::array<float, 2> sight;
    std::array<ColourAllele, 2> colour;
    std::array<SexChromosome, 2> sexChromosomes;
};

// One allele per gene, as carried by a sperm or egg.
struct Gamete {
    float sight;
    ColourAllele colour;
    SexChromosome sexChromosome;
};

// Creates a founder genome with random sight and colour and chromosomes
// matching the requested sex, giving the population its initial variation.
Genome randomGenome(Sex sex, const GeneticsConfig& cfg, Rng& rng);

// Produces a gamete by Mendelian segregation followed by optional mutation;
// the sex chromosome is passed through unchanged so the father sets the sex.
Gamete makeGamete(const Genome& g, const GeneticsConfig& cfg, Rng& rng);

// Combines two gametes with the mother in slot 0 and the father in slot 1.
Genome fertilise(const Gamete& fromMother, const Gamete& fromFather);

// Performs one complete breeding event and returns the child genome.
Genome cross(const Genome& mother, const Genome& father,
             const GeneticsConfig& cfg, Rng& rng);

// Returns Male when a Y chromosome is present, otherwise Female.
Sex sexOf(const Genome& g);

// Returns the expressed sight: the mean of the two sight alleles.
float sightOf(const Genome& g);

// Returns the visible colour after complete dominance.
ColourPhenotype colourOf(const Genome& g);

// Formats a genome on one line for debugging and demo output.
std::string describe(const Genome& g);

}  // namespace genetics
