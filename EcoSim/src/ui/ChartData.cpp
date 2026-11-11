// Chart data implementation; all work here is graphics-independent and tested
// headlessly.
#include "ui/ChartData.h"

#include <cstddef>
#include <memory>

#include "sim/Organism.h"
#include "sim/Simulation.h"
#include "sim/Stats.h"

void buildChartSeries(const StatsManager& stats, ChartSeries& out) {
    out.time.clear();
    out.plants.clear();
    out.herbivores.clear();
    out.predators.clear();
    out.herbMeanSight.clear();
    out.predMeanSight.clear();
    out.herbDarkAlleleFreq.clear();
    out.predDarkAlleleFreq.clear();

    const std::vector<StatsSample>& samples = stats.samples();
    out.time.reserve(samples.size());
    out.plants.reserve(samples.size());
    out.herbivores.reserve(samples.size());
    out.predators.reserve(samples.size());
    out.herbMeanSight.reserve(samples.size());
    out.predMeanSight.reserve(samples.size());
    out.herbDarkAlleleFreq.reserve(samples.size());
    out.predDarkAlleleFreq.reserve(samples.size());

    for (const StatsSample& sample : samples) {
        out.time.push_back(sample.time);
        out.plants.push_back(static_cast<double>(sample.plants));
        out.herbivores.push_back(
            static_cast<double>(sample.herbivores.count));
        out.predators.push_back(
            static_cast<double>(sample.predators.count));
        out.herbMeanSight.push_back(
            static_cast<double>(sample.herbivores.meanSight));
        out.predMeanSight.push_back(
            static_cast<double>(sample.predators.meanSight));
        out.herbDarkAlleleFreq.push_back(
            static_cast<double>(sample.herbivores.darkAlleleFreq));
        out.predDarkAlleleFreq.push_back(
            static_cast<double>(sample.predators.darkAlleleFreq));
    }
}

std::vector<float> livingSights(const Simulation& sim, Kind kind) {
    std::vector<float> sights;
    if (kind == Kind::Plant) {
        return sights;
    }

    sights.reserve(static_cast<std::size_t>(sim.count(kind)));
    for (const std::unique_ptr<Organism>& organism : sim.organisms()) {
        if (!organism->alive || organism->kind() != kind) {
            continue;
        }
        const Animal* animal = dynamic_cast<const Animal*>(organism.get());
        if (animal != nullptr) {
            sights.push_back(animal->sight);
        }
    }
    return sights;
}
