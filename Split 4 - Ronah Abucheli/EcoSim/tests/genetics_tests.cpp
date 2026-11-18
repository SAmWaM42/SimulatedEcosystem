// Unit tests for the genetics core and RNG properties; all randomness uses
// fixed seeds and assertions check properties rather than exact values.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <array>

#include "core/Rng.h"
#include "genetics/Genetics.h"

namespace {
// Builds a genome by hand for deterministic trait-expression tests.
genetics::Genome makeGenome(std::array<float, 2> sight,
                            std::array<genetics::ColourAllele, 2> colour,
                            std::array<genetics::SexChromosome, 2> sexChromosomes) {
    genetics::Genome genome;
    genome.sight = sight;
    genome.colour = colour;
    genome.sexChromosomes = sexChromosomes;
    return genome;
}

// Disables mutation so inheritance tests inspect pure segregation.
genetics::GeneticsConfig noMutationConfig() {
    genetics::GeneticsConfig cfg;
    cfg.sightMutationRate = 0.f;
    cfg.colourMutationRate = 0.f;
    return cfg;
}
}  // namespace

TEST_CASE("founder genomes are valid") {
    Rng rng(1);
    const genetics::GeneticsConfig cfg;

    for (int i = 0; i < 1000; ++i) {
        const genetics::Genome female = genetics::randomGenome(genetics::Sex::Female, cfg, rng);
        CHECK(female.sight[0] >= cfg.sightMin);
        CHECK(female.sight[0] < cfg.sightMax);
        CHECK(female.sight[1] >= cfg.sightMin);
        CHECK(female.sight[1] < cfg.sightMax);
        CHECK(female.sexChromosomes[0] == genetics::SexChromosome::X);
        CHECK(female.sexChromosomes[1] == genetics::SexChromosome::X);
        CHECK(genetics::sexOf(female) == genetics::Sex::Female);

        const genetics::Genome male = genetics::randomGenome(genetics::Sex::Male, cfg, rng);
        CHECK(male.sight[0] >= cfg.sightMin);
        CHECK(male.sight[0] < cfg.sightMax);
        CHECK(male.sight[1] >= cfg.sightMin);
        CHECK(male.sight[1] < cfg.sightMax);
        CHECK(male.sexChromosomes[0] == genetics::SexChromosome::X);
        CHECK(male.sexChromosomes[1] == genetics::SexChromosome::Y);
        CHECK(genetics::sexOf(male) == genetics::Sex::Male);
    }
}

TEST_CASE("sightOf averages the two alleles") {
    const genetics::Genome genome =
        makeGenome({280.f, 360.f},
                   {genetics::ColourAllele::Medium, genetics::ColourAllele::Light},
                   {genetics::SexChromosome::X, genetics::SexChromosome::X});
    CHECK(genetics::sightOf(genome) == doctest::Approx(320.0f));
}

TEST_CASE("colourOf applies complete dominance") {
    const struct {
        genetics::ColourAllele first;
        genetics::ColourAllele second;
        genetics::ColourPhenotype expected;
    } cases[] = {
        {genetics::ColourAllele::Dark, genetics::ColourAllele::Dark, genetics::ColourPhenotype::Dark},
        {genetics::ColourAllele::Dark, genetics::ColourAllele::Medium, genetics::ColourPhenotype::Dark},
        {genetics::ColourAllele::Dark, genetics::ColourAllele::Light, genetics::ColourPhenotype::Dark},
        {genetics::ColourAllele::Medium, genetics::ColourAllele::Medium, genetics::ColourPhenotype::Medium},
        {genetics::ColourAllele::Medium, genetics::ColourAllele::Light, genetics::ColourPhenotype::Medium},
        {genetics::ColourAllele::Light, genetics::ColourAllele::Light, genetics::ColourPhenotype::Light},
    };

    for (const auto& testCase : cases) {
        const genetics::Genome genome =
            makeGenome({300.f, 300.f}, {testCase.first, testCase.second},
                       {genetics::SexChromosome::X, genetics::SexChromosome::X});
        CHECK(genetics::colourOf(genome) == testCase.expected);
    }
}

TEST_CASE("inheritance without mutation follows parental alleles") {
    const genetics::GeneticsConfig cfg = noMutationConfig();
    Rng rng(2);

    for (int i = 0; i < 1000; ++i) {
        const genetics::Genome mother = genetics::randomGenome(genetics::Sex::Female, cfg, rng);
        const genetics::Genome father = genetics::randomGenome(genetics::Sex::Male, cfg, rng);
        const genetics::Genome child = genetics::cross(mother, father, cfg, rng);

        CHECK((child.sight[0] == mother.sight[0] || child.sight[0] == mother.sight[1]));
        CHECK((child.sight[1] == father.sight[0] || child.sight[1] == father.sight[1]));
        CHECK((child.colour[0] == mother.colour[0] || child.colour[0] == mother.colour[1]));
        CHECK((child.colour[1] == father.colour[0] || child.colour[1] == father.colour[1]));
        CHECK(child.sexChromosomes[0] == genetics::SexChromosome::X);
    }
}

TEST_CASE("sex ratio is close to one half") {
    const genetics::GeneticsConfig cfg = noMutationConfig();
    Rng rng(3);
    const genetics::Genome mother = genetics::randomGenome(genetics::Sex::Female, cfg, rng);
    const genetics::Genome father = genetics::randomGenome(genetics::Sex::Male, cfg, rng);

    int females = 0;
    for (int i = 0; i < 10000; ++i) {
        if (genetics::sexOf(genetics::cross(mother, father, cfg, rng)) == genetics::Sex::Female) {
            ++females;
        }
    }

    const double fraction = static_cast<double>(females) / 10000.0;
    CHECK(fraction >= 0.47);
    CHECK(fraction <= 0.53);
}

TEST_CASE("heterozygous dark by light gives a 3:1 ratio") {
    const genetics::GeneticsConfig cfg = noMutationConfig();
    Rng rng(4);
    const genetics::Genome mother =
        makeGenome({300.f, 300.f},
                   {genetics::ColourAllele::Dark, genetics::ColourAllele::Light},
                   {genetics::SexChromosome::X, genetics::SexChromosome::X});
    const genetics::Genome father =
        makeGenome({300.f, 300.f},
                   {genetics::ColourAllele::Dark, genetics::ColourAllele::Light},
                   {genetics::SexChromosome::X, genetics::SexChromosome::Y});

    int dark = 0;
    for (int i = 0; i < 20000; ++i) {
        const genetics::Genome child = genetics::cross(mother, father, cfg, rng);
        if (genetics::colourOf(child) == genetics::ColourPhenotype::Dark) {
            ++dark;
        } else {
            CHECK(genetics::colourOf(child) == genetics::ColourPhenotype::Light);
        }
    }

    const double fraction = static_cast<double>(dark) / 20000.0;
    CHECK(fraction >= 0.73);
    CHECK(fraction <= 0.77);
}

TEST_CASE("sight mutation respects bounds and changes alleles") {
    Rng rng(5);
    const genetics::Genome mother =
        makeGenome({200.f, 400.f},
                   {genetics::ColourAllele::Dark, genetics::ColourAllele::Dark},
                   {genetics::SexChromosome::X, genetics::SexChromosome::X});
    const genetics::Genome father =
        makeGenome({200.f, 400.f},
                   {genetics::ColourAllele::Dark, genetics::ColourAllele::Dark},
                   {genetics::SexChromosome::X, genetics::SexChromosome::Y});

    genetics::GeneticsConfig mutated;
    mutated.sightMutationRate = 1.f;
    mutated.sightMutationSd = 100.f;
    mutated.colourMutationRate = 0.f;

    const int childCount = 2000;
    int changedChildren = 0;
    int changedAlleles = 0;
    for (int i = 0; i < childCount; ++i) {
        const genetics::Genome child = genetics::cross(mother, father, mutated, rng);
        CHECK(child.sight[0] >= mutated.sightMin);
        CHECK(child.sight[0] <= mutated.sightMax);
        CHECK(child.sight[1] >= mutated.sightMin);
        CHECK(child.sight[1] <= mutated.sightMax);

        const bool firstChanged = child.sight[0] != mother.sight[0] &&
                                  child.sight[0] != mother.sight[1];
        const bool secondChanged = child.sight[1] != father.sight[0] &&
                                   child.sight[1] != father.sight[1];
        if (firstChanged || secondChanged) {
            ++changedChildren;
        }
        if (firstChanged) {
            ++changedAlleles;
        }
        if (secondChanged) {
            ++changedAlleles;
        }
    }

    CHECK(static_cast<double>(changedChildren) / childCount > 0.6);
    CHECK(static_cast<double>(changedAlleles) / (2 * childCount) > 0.4);

    genetics::GeneticsConfig unmutated = mutated;
    unmutated.sightMutationRate = 0.f;
    Rng unmutatedRng(6);
    for (int i = 0; i < 1000; ++i) {
        const genetics::Genome child = genetics::cross(mother, father, unmutated, unmutatedRng);
        CHECK((child.sight[0] == 200.f || child.sight[0] == 400.f));
        CHECK((child.sight[1] == 200.f || child.sight[1] == 400.f));
    }
}

TEST_CASE("colour mutation always changes the allele") {
    genetics::GeneticsConfig cfg;
    cfg.sightMutationRate = 0.f;
    cfg.colourMutationRate = 1.f;
    Rng rng(7);
    const genetics::Genome parent =
        makeGenome({300.f, 300.f},
                   {genetics::ColourAllele::Dark, genetics::ColourAllele::Dark},
                   {genetics::SexChromosome::X, genetics::SexChromosome::X});

    for (int i = 0; i < 1000; ++i) {
        const genetics::Gamete gamete = genetics::makeGamete(parent, cfg, rng);
        CHECK(gamete.colour != genetics::ColourAllele::Dark);
        CHECK((gamete.colour == genetics::ColourAllele::Medium ||
               gamete.colour == genetics::ColourAllele::Light));
    }
}

TEST_CASE("Rng is reproducible for a seed") {
    Rng first(123);
    Rng second(123);
    for (int i = 0; i < 10; ++i) {
        CHECK(first.uniform(0.f, 1.f) == second.uniform(0.f, 1.f));
    }
}
