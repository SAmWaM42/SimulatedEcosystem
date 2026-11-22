// Console demo that exercises inheritance, mutation, and sight-based selection
// over a toy population; it is headless and independent of the renderer.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

#include "core/Rng.h"
#include "genetics/Genetics.h"

namespace {
constexpr int kPopulationSize = 200;

// Toy fitness peaks at sight 360 and ignores colour, standing in for survival.
double fitness(float sight) {
    const double distance = (static_cast<double>(sight) - 360.0) / 60.0;
    return std::exp(-(distance * distance));
}

// Builds half female and half male founders from random genomes.
std::vector<genetics::Genome> makeFounderPopulation(
    const genetics::GeneticsConfig& cfg, Rng& rng) {
    std::vector<genetics::Genome> population;
    population.reserve(kPopulationSize);
    for (int i = 0; i < kPopulationSize / 2; ++i) {
        population.push_back(genetics::randomGenome(genetics::Sex::Female, cfg, rng));
        population.push_back(genetics::randomGenome(genetics::Sex::Male, cfg, rng));
    }
    return population;
}

// Creates the next generation by fitness-weighted parent selection.
// Requires at least one female and one male in the population.
std::vector<genetics::Genome> nextGeneration(
    const std::vector<genetics::Genome>& population,
    const genetics::GeneticsConfig& cfg, Rng& rng) {
    std::vector<genetics::Genome> females;
    std::vector<genetics::Genome> males;
    std::vector<double> femaleWeights;
    std::vector<double> maleWeights;
    females.reserve(population.size());
    males.reserve(population.size());
    femaleWeights.reserve(population.size());
    maleWeights.reserve(population.size());

    for (const genetics::Genome& genome : population) {
        const double weight = fitness(genetics::sightOf(genome));
        if (genetics::sexOf(genome) == genetics::Sex::Female) {
            females.push_back(genome);
            femaleWeights.push_back(weight);
        } else {
            males.push_back(genome);
            maleWeights.push_back(weight);
        }
    }

    std::discrete_distribution<std::size_t> femalePicker(
        femaleWeights.begin(), femaleWeights.end());
    std::discrete_distribution<std::size_t> malePicker(
        maleWeights.begin(), maleWeights.end());

    std::vector<genetics::Genome> children;
    children.reserve(kPopulationSize);
    for (int i = 0; i < kPopulationSize; ++i) {
        const genetics::Genome& mother = females[femalePicker(rng.engine())];
        const genetics::Genome& father = males[malePicker(rng.engine())];
        children.push_back(genetics::cross(mother, father, cfg, rng));
    }
    return children;
}

// Prints one population-statistics row with fixed column widths.
void printRow(int generation, const std::vector<genetics::Genome>& population) {
    double sightSum = 0.0;
    int dark = 0;
    int medium = 0;
    int light = 0;
    int female = 0;

    for (const genetics::Genome& genome : population) {
        sightSum += genetics::sightOf(genome);
        switch (genetics::colourOf(genome)) {
        case genetics::ColourPhenotype::Dark:
            ++dark;
            break;
        case genetics::ColourPhenotype::Medium:
            ++medium;
            break;
        case genetics::ColourPhenotype::Light:
            ++light;
            break;
        }
        if (genetics::sexOf(genome) == genetics::Sex::Female) {
            ++female;
        }
    }

    const double total = static_cast<double>(population.size());
    const double mean = sightSum / total;

    // Population standard deviation (divide by N), since this is the whole
    // simulated population rather than a sample estimate.
    double squaredDeviationSum = 0.0;
    for (const genetics::Genome& genome : population) {
        const double deviation = genetics::sightOf(genome) - mean;
        squaredDeviationSum += deviation * deviation;
    }
    const double sd = std::sqrt(squaredDeviationSum / total);

    std::printf("%4d %10.2f %9.2f %7.1f%% %7.1f%% %7.1f%% %7.1f%%\n",
                generation, mean, sd, 100.0 * dark / total,
                100.0 * medium / total, 100.0 * light / total,
                100.0 * female / total);
}
}  // namespace

// Runs the toy population for the requested seed and generation count.
int main(int argc, char** argv) {
    std::uint32_t seed = 1;
    int generations = 100;
    if (argc > 1) {
        seed = static_cast<std::uint32_t>(std::stoul(argv[1]));
    }
    if (argc > 2) {
        generations = std::stoi(argv[2]);
    }
    if (generations < 0) {
        generations = 0;
    }

    Rng rng(seed);
    const genetics::GeneticsConfig cfg;
    std::vector<genetics::Genome> population = makeFounderPopulation(cfg, rng);

    std::printf("%4s %10s %9s %8s %8s %8s %8s\n",
                "gen", "meanSight", "sdSight", "%Dark", "%Medium",
                "%Light", "%Female");

    for (int generation = 0;; ++generation) {
        bool hasFemale = false;
        bool hasMale = false;
        for (const genetics::Genome& genome : population) {
            if (genetics::sexOf(genome) == genetics::Sex::Female) {
                hasFemale = true;
            } else {
                hasMale = true;
            }
        }
        if (!hasFemale || !hasMale) {
            std::printf("Generation %d has no %s; stopping.\n", generation,
                        !hasFemale ? "females" : "males");
            break;
        }

        if (generation == 0 || generation % 10 == 0 || generation == generations) {
            printRow(generation, population);
        }
        if (generation == generations) {
            break;
        }
        population = nextGeneration(population, cfg, rng);
    }
    return 0;
}
