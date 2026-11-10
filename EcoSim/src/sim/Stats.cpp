// Stats implementation: aggregates one kind, snapshots cumulative counters, and
// writes the documented CSV columns.
#include "sim/Stats.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <ostream>
#include <string>

#include "sim/Simulation.h"

namespace {
// Summarises all living organisms of one kind into a KindStats record.
KindStats gatherKindStats(const Simulation& sim, Kind kind) {
    KindStats stats;
    double sightSum = 0.0;
    int darkAlleles = 0;
    bool firstSight = true;

    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        if (!organism->alive || organism->kind() != kind) {
            continue;
        }
        const Animal* animal = dynamic_cast<const Animal*>(organism.get());
        if (animal == nullptr) {
            continue;
        }

        ++stats.count;
        if (animal->sex == genetics::Sex::Female) {
            ++stats.females;
        } else {
            ++stats.males;
        }

        sightSum += static_cast<double>(animal->sight);
        if (firstSight) {
            stats.minSight = animal->sight;
            stats.maxSight = animal->sight;
            firstSight = false;
        } else {
            stats.minSight = std::min(stats.minSight, animal->sight);
            stats.maxSight = std::max(stats.maxSight, animal->sight);
        }

        switch (genetics::colourOf(animal->genome)) {
        case genetics::ColourPhenotype::Dark:
            ++stats.darkCount;
            break;
        case genetics::ColourPhenotype::Medium:
            ++stats.mediumCount;
            break;
        case genetics::ColourPhenotype::Light:
            ++stats.lightCount;
            break;
        }

        for (genetics::ColourAllele allele : animal->genome.colour) {
            if (allele == genetics::ColourAllele::Dark) {
                ++darkAlleles;
            }
        }
    }

    if (stats.count > 0) {
        stats.meanSight = static_cast<float>(
            sightSum / static_cast<double>(stats.count));
        stats.darkAlleleFreq = static_cast<float>(darkAlleles) /
                               static_cast<float>(2 * stats.count);
    }
    return stats;
}

// Returns the highest generation among living animals.
int maxLivingGeneration(const Simulation& sim) {
    int maximum = 0;
    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        const Animal* animal = dynamic_cast<const Animal*>(organism.get());
        if (animal != nullptr && animal->alive) {
            maximum = std::max(maximum, animal->generation);
        }
    }
    return maximum;
}

// Writes the shared KindStats columns without a trailing newline.
void writeKindStats(std::ostream& out, const KindStats& stats) {
    out << stats.count << ',' << stats.females << ',' << stats.males << ','
        << stats.meanSight << ',' << stats.minSight << ',' << stats.maxSight
        << ',' << stats.darkCount << ',' << stats.mediumCount << ','
        << stats.lightCount << ',' << stats.darkAlleleFreq << ',';
}
}  // namespace

void StatsManager::sample(const Simulation& sim) {
    StatsSample sample;
    sample.time = sim.time();
    sample.plants = sim.count(Kind::Plant);
    sample.herbivores = gatherKindStats(sim, Kind::Herbivore);
    sample.predators = gatherKindStats(sim, Kind::Predator);
    sample.births = sim.birthCount();
    sample.deathsStarved =
        sim.deathCount(Kind::Herbivore, DeathCause::Starved) +
        sim.deathCount(Kind::Predator, DeathCause::Starved);
    sample.deathsOldAge =
        sim.deathCount(Kind::Herbivore, DeathCause::OldAge) +
        sim.deathCount(Kind::Predator, DeathCause::OldAge);
    sample.deathsEaten =
        sim.deathCount(Kind::Herbivore, DeathCause::Eaten) +
        sim.deathCount(Kind::Predator, DeathCause::Eaten);
    sample.immigrants = sim.immigrantCount();
    sample.maxGeneration = maxLivingGeneration(sim);
    sampleList.push_back(sample);
}

const std::vector<StatsSample>& StatsManager::samples() const {
    return sampleList;
}

bool StatsManager::writeCsv(const std::string& path) const {
    std::ofstream out(path);
    if (!out) {
        return false;
    }

    out << "time,plants,"
        << "herb_count,herb_females,herb_males,herb_meanSight,herb_minSight,"
        << "herb_maxSight,herb_darkCount,herb_mediumCount,herb_lightCount,"
        << "herb_darkAlleleFreq,"
        << "pred_count,pred_females,pred_males,pred_meanSight,pred_minSight,"
        << "pred_maxSight,pred_darkCount,pred_mediumCount,pred_lightCount,"
        << "pred_darkAlleleFreq,"
        << "births,deathsStarved,deathsOldAge,deathsEaten,immigrants,"
        << "maxGeneration\n";

    for (const StatsSample& sample : sampleList) {
        out << sample.time << ',' << sample.plants << ',';
        writeKindStats(out, sample.herbivores);
        writeKindStats(out, sample.predators);
        out << sample.births << ',' << sample.deathsStarved << ','
            << sample.deathsOldAge << ',' << sample.deathsEaten << ','
            << sample.immigrants << ',' << sample.maxGeneration << '\n';
    }

    return out.good();
}
