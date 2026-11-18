// Implementation of the genetics core: founder genomes, Mendelian segregation,
// mutation, fertilisation, and trait expression.
#include "genetics/Genetics.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iomanip>
#include <sstream>

namespace genetics {
namespace {
// Maps a sex chromosome to its display character.
char chromosomeName(SexChromosome chromosome) {
    return chromosome == SexChromosome::X ? 'X' : 'Y';
}

// Maps a colour allele to its display name.
const char* alleleName(ColourAllele allele) {
    switch (allele) {
    case ColourAllele::Dark:
        return "Dark";
    case ColourAllele::Medium:
        return "Medium";
    case ColourAllele::Light:
        return "Light";
    }
    return "Unknown";
}

// Maps a colour phenotype to its display name.
const char* phenotypeName(ColourPhenotype phenotype) {
    switch (phenotype) {
    case ColourPhenotype::Dark:
        return "Dark";
    case ColourPhenotype::Medium:
        return "Medium";
    case ColourPhenotype::Light:
        return "Light";
    }
    return "Unknown";
}

// Keeps sight inside the configured inclusive range after mutation.
float clampSight(float value, const GeneticsConfig& cfg) {
    return std::max(cfg.sightMin, std::min(value, cfg.sightMax));
}

// Picks one of two alleles with equal probability for Mendelian segregation.
std::size_t pickAlleleIndex(Rng& rng) {
    return static_cast<std::size_t>(rng.uniformInt(0, 1));
}
}  // namespace

Genome randomGenome(Sex sex, const GeneticsConfig& cfg, Rng& rng) {
    Genome genome;
    genome.sight = {rng.uniform(cfg.sightMin, cfg.sightMax),
                    rng.uniform(cfg.sightMin, cfg.sightMax)};
    genome.colour = {static_cast<ColourAllele>(rng.uniformInt(0, 2)),
                     static_cast<ColourAllele>(rng.uniformInt(0, 2))};

    if (sex == Sex::Female) {
        genome.sexChromosomes = {SexChromosome::X, SexChromosome::X};
    } else {
        genome.sexChromosomes = {SexChromosome::X, SexChromosome::Y};
    }
    return genome;
}

Gamete makeGamete(const Genome& g, const GeneticsConfig& cfg, Rng& rng) {
    Gamete gamete;
    gamete.sight = g.sight[pickAlleleIndex(rng)];
    gamete.colour = g.colour[pickAlleleIndex(rng)];
    gamete.sexChromosome = g.sexChromosomes[pickAlleleIndex(rng)];

    if (rng.chance(cfg.sightMutationRate)) {
        gamete.sight = clampSight(
            gamete.sight + rng.normal(0.0f, cfg.sightMutationSd), cfg);
    }

    if (rng.chance(cfg.colourMutationRate)) {
        // Shift by one or two places so the mutation always changes the allele.
        const int shifted =
            (static_cast<int>(gamete.colour) + rng.uniformInt(1, 2)) % 3;
        gamete.colour = static_cast<ColourAllele>(shifted);
    }

    return gamete;
}

Genome fertilise(const Gamete& fromMother, const Gamete& fromFather) {
    Genome child;
    child.sight = {fromMother.sight, fromFather.sight};
    child.colour = {fromMother.colour, fromFather.colour};
    child.sexChromosomes = {fromMother.sexChromosome, fromFather.sexChromosome};
    return child;
}

Genome cross(const Genome& mother, const Genome& father,
             const GeneticsConfig& cfg, Rng& rng) {
    assert(sexOf(mother) == Sex::Female);
    assert(sexOf(father) == Sex::Male);

    // Evaluate gametes in a fixed order so runs are reproducible across compilers.
    const Gamete egg = makeGamete(mother, cfg, rng);
    const Gamete sperm = makeGamete(father, cfg, rng);
    return fertilise(egg, sperm);
}

Sex sexOf(const Genome& g) {
    const bool hasY = g.sexChromosomes[0] == SexChromosome::Y ||
                      g.sexChromosomes[1] == SexChromosome::Y;
    return hasY ? Sex::Male : Sex::Female;
}

float sightOf(const Genome& g) {
    return (g.sight[0] + g.sight[1]) * 0.5f;
}

ColourPhenotype colourOf(const Genome& g) {
    if (g.colour[0] == ColourAllele::Dark || g.colour[1] == ColourAllele::Dark) {
        return ColourPhenotype::Dark;
    }
    if (g.colour[0] == ColourAllele::Medium ||
        g.colour[1] == ColourAllele::Medium) {
        return ColourPhenotype::Medium;
    }
    return ColourPhenotype::Light;
}

std::string describe(const Genome& g) {
    std::ostringstream out;
    out << chromosomeName(g.sexChromosomes[0])
        << chromosomeName(g.sexChromosomes[1])
        << " | sight " << std::fixed << std::setprecision(1) << sightOf(g)
        << " (" << g.sight[0] << "/" << g.sight[1] << ")"
        << " | colour " << phenotypeName(colourOf(g))
        << " (" << alleleName(g.colour[0]) << "/" << alleleName(g.colour[1])
        << ")";
    return out.str();
}

}  // namespace genetics
