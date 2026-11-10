// Per-sample population statistics and CSV export for headless runs and the
// later UI; animals are summarised by sight, sex, colour, and generation.
#pragma once

#include <string>
#include <vector>

class Simulation;

// Aggregate state of one animal kind in a single sample.
struct KindStats {
    int count = 0;
    int females = 0;
    int males = 0;
    float meanSight = 0.f;
    float minSight = 0.f;
    float maxSight = 0.f;
    int darkCount = 0;
    int mediumCount = 0;
    int lightCount = 0;
    // Fraction of colour alleles (two per animal) that are Dark; zero when empty.
    float darkAlleleFreq = 0.f;
};

// One timestamped snapshot of the simulation; counters are cumulative animals.
struct StatsSample {
    double time = 0.0;
    int plants = 0;
    KindStats herbivores;
    KindStats predators;
    int births = 0;
    int deathsStarved = 0;
    int deathsOldAge = 0;
    int deathsEaten = 0;
    int immigrants = 0;
    int maxGeneration = 0;  // highest generation among living animals
};

// Collects samples and writes them as a CSV table for external analysis.
class StatsManager {
public:
    // Appends one snapshot computed from the current living population.
    void sample(const Simulation& sim);
    // Returns every sample in chronological order.
    const std::vector<StatsSample>& samples() const;
    // Writes a header and one row per sample; returns false if the file fails.
    bool writeCsv(const std::string& path) const;

private:
    std::vector<StatsSample> sampleList;
};
