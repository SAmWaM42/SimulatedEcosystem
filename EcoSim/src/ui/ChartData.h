// GL-free chart data preparation: copies StatsManager samples into plot-ready
// vectors and collects the current sight values of living animals.
#pragma once

#include <vector>

enum class Kind;

class Simulation;
class StatsManager;

// One value per stats sample, in chronological order.
struct ChartSeries {
    std::vector<double> time;
    std::vector<double> plants;
    std::vector<double> herbivores;
    std::vector<double> predators;
    std::vector<double> herbMeanSight;
    std::vector<double> predMeanSight;
    std::vector<double> herbDarkAlleleFreq;
    std::vector<double> predDarkAlleleFreq;
};

// Clears and fills every series from the stats history.
void buildChartSeries(const StatsManager& stats, ChartSeries& out);

// Returns the expressed sight of every living animal of the requested kind.
std::vector<float> livingSights(const Simulation& sim, Kind kind);
