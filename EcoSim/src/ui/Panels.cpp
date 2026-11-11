// ImGui/ImPlot panel implementations: controls and settings, HUD, organism
// inspector with lineage links, and the statistics charts.
#include "ui/Panels.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>
#include <vector>

#include <imgui.h>
#include <implot.h>

#include <glm/vec3.hpp>

#include "genetics/Genetics.h"
#include "render/Palette.h"
#include "sim/Lineage.h"
#include "sim/Organism.h"
#include "sim/SimClock.h"
#include "sim/SimConfig.h"
#include "sim/Simulation.h"
#include "sim/Stats.h"
#include "ui/ChartData.h"
#include "ui/UiText.h"

namespace {
constexpr float kPanelMargin = 16.f;
constexpr float kControlsWidth = 370.f;
constexpr float kInspectorTop = 340.f;

// Converts the renderer palette's linear-ish RGB to an ImGui colour.
ImVec4 panelColour(const glm::vec3& colour) {
    return ImVec4(colour.r, colour.g, colour.b, 1.f);
}

// Returns the display name used for an animal's sex.
const char* sexName(genetics::Sex sex) {
    return sex == genetics::Sex::Female ? "Female" : "Male";
}

// Draws an alive parent as a clickable id and a dead or missing parent as a
// disabled button so the family section never selects an invalid organism.
void parentButton(const Simulation& sim, std::uint32_t id,
                  UiActions& actions) {
    if (id == 0) {
        ImGui::TextUnformatted("Founder");
        return;
    }

    const bool alive = sim.find(id) != nullptr;
    std::string label = "#" + std::to_string(id);
    if (!alive) {
        label += " (dead)";
    }

    ImGui::BeginDisabled(!alive);
    if (ImGui::Button(label.c_str()) && alive) {
        actions.selectId = id;
    }
    ImGui::EndDisabled();
}

// Draws the settings half of the controls window and applies live changes to
// the running simulation immediately.
void drawSettingsSection(SimConfig& runConfig, Simulation& sim,
                         std::uint32_t seed) {
    bool liveChanged = false;

    // Reserve room for labels to the right of the value sliders.
    ImGui::PushItemWidth(160.f);

    if (ImGui::SliderFloat("Sight mutation rate",
                           &runConfig.genetics.sightMutationRate, 0.f, 0.5f,
                           "%.3f")) {
        liveChanged = true;
    }
    if (ImGui::SliderFloat("Sight mutation SD",
                           &runConfig.genetics.sightMutationSd, 0.f, 50.f,
                           "%.2f")) {
        liveChanged = true;
    }
    if (ImGui::SliderFloat("Colour mutation rate",
                           &runConfig.genetics.colourMutationRate, 0.f, 0.2f,
                           "%.3f")) {
        liveChanged = true;
    }
    if (ImGui::SliderFloat("Sight upkeep", &runConfig.sightUpkeep, 0.f, 0.02f,
                           "%.4f")) {
        liveChanged = true;
    }

    if (ImGui::SliderFloat("Plant spawn min (s)", &runConfig.plantSpawnMin,
                           2.f, 60.f, "%.1f")) {
        if (runConfig.plantSpawnMin > runConfig.plantSpawnMax) {
            runConfig.plantSpawnMax = runConfig.plantSpawnMin;
        }
        liveChanged = true;
    }
    if (ImGui::SliderFloat("Plant spawn max (s)", &runConfig.plantSpawnMax,
                           2.f, 60.f, "%.1f")) {
        if (runConfig.plantSpawnMax < runConfig.plantSpawnMin) {
            runConfig.plantSpawnMin = runConfig.plantSpawnMax;
        }
        liveChanged = true;
    }
    if (ImGui::SliderFloat("Plant energy", &runConfig.plantEnergyValue, 5.f,
                           80.f, "%.1f")) {
        liveChanged = true;
    }
    if (ImGui::SliderFloat("Predator catch", &runConfig.predatorCatchChance,
                           0.f, 1.f, "%.2f")) {
        liveChanged = true;
    }
    if (ImGui::Checkbox("Enable rescue", &runConfig.enableRescue)) {
        liveChanged = true;
    }

    if (liveChanged) {
        copyLiveTunables(runConfig, sim.tunables());
    }

    ImGui::SeparatorText("Applies on reset");
    ImGui::SliderInt("Initial plants", &runConfig.initialPlants, 0, 1000);

    int herbivores = runConfig.initialHerbivores;
    if (ImGui::SliderInt("Initial herbivores", &herbivores, 2, 150)) {
        runConfig.initialHerbivores = herbivores - (herbivores % 2);
    }

    int predators = runConfig.initialPredators;
    if (ImGui::SliderInt("Initial predators", &predators, 0, 50)) {
        runConfig.initialPredators = predators - (predators % 2);
    }

    ImGui::Text("Seed: %u", static_cast<unsigned int>(seed));
    ImGui::PopItemWidth();
}

// The statistics panels rebuild their series only when the sample history
// changes; one app instance owns one ImGui/ImPlot context, so panel-local
// caching is sufficient.
struct StatisticsCache {
    std::size_t sampleCount = 0;
    double lastSampleTime = -1.0;
    ChartSeries series;
};

StatisticsCache& statisticsCache() {
    static StatisticsCache cache;
    return cache;
}
}  // namespace

void drawControlsPanel(SimClock& clock, bool followSelected, bool hasSelection,
                       bool& showSightRing, SimConfig& runConfig,
                       Simulation& sim, std::uint32_t seed, UiActions& actions) {
    ImGui::SetNextWindowPos(ImVec2(kPanelMargin, kPanelMargin),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(kControlsWidth, 300.f),
                             ImGuiCond_FirstUseEver);
    ImGui::Begin("Simulation");

    if (ImGui::Button(clock.paused() ? "Play" : "Pause")) {
        actions.togglePause = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset")) {
        actions.restart = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("New seed")) {
        actions.newSeed = true;
    }

    float speed = clock.speed();
    if (ImGui::SliderFloat("Speed", &speed, 0.25f, 16.f, "%.2fx",
                           ImGuiSliderFlags_Logarithmic)) {
        clock.setSpeed(speed);
    }

    bool follow = followSelected;
    ImGui::BeginDisabled(!hasSelection);
    if (ImGui::Checkbox("Follow selected", &follow) &&
        follow != followSelected) {
        actions.toggleFollow = true;
    }
    ImGui::EndDisabled();
    ImGui::Checkbox("Show sight ring", &showSightRing);

    ImGui::Spacing();
    if (ImGui::CollapsingHeader("Settings")) {
        drawSettingsSection(runConfig, sim, seed);
    }

    ImGui::End();
}

void drawHud(const HudData& hud) {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(viewport->WorkPos.x + viewport->WorkSize.x - kPanelMargin,
               viewport->WorkPos.y + kPanelMargin),
        ImGuiCond_Always, ImVec2(1.f, 0.f));
    ImGui::SetNextWindowBgAlpha(0.55f);
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_AlwaysAutoResize;

    ImGui::Begin("##hud", nullptr, flags);
    ImGui::Text("Time  %s", formatClock(hud.time).c_str());
    ImGui::Text("Generation  %d", hud.generation);
    ImGui::Text("Plants  %d", hud.plants);
    ImGui::Text("Herbivores  %d (F %d / M %d)",
                hud.herbivoreFemales + hud.herbivoreMales,
                hud.herbivoreFemales, hud.herbivoreMales);
    ImGui::Text("Predators  %d (F %d / M %d)",
                hud.predatorFemales + hud.predatorMales, hud.predatorFemales,
                hud.predatorMales);
    ImGui::Text("FPS  %d", hud.fps);
    if (hud.paused) {
        ImGui::TextColored(ImVec4(1.f, 0.85f, 0.30f, 1.f), "PAUSED");
    }
    ImGui::End();
}

void drawInspectorPanel(const Simulation& sim, std::uint32_t selectedId,
                        UiActions& actions) {
    ImGui::SetNextWindowPos(ImVec2(kPanelMargin, kInspectorTop),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(kControlsWidth, 350.f),
                             ImGuiCond_FirstUseEver);
    ImGui::Begin("Selected organism");

    const Organism* selected =
        selectedId != 0 ? sim.find(selectedId) : nullptr;
    if (selected == nullptr) {
        ImGui::TextUnformatted("Click an organism in the 3D view.");
        ImGui::End();
        return;
    }

    ImGui::Text("Kind: %s", kindName(selected->kind()).c_str());
    ImGui::Text("Id: #%u", static_cast<unsigned int>(selected->id));

    if (selected->kind() == Kind::Plant) {
        ImGui::Text("Age: %.1f s", selected->age);
        ImGui::End();
        return;
    }

    const Animal* animal = dynamic_cast<const Animal*>(selected);
    if (animal == nullptr) {
        ImGui::TextDisabled("No animal data available.");
        ImGui::End();
        return;
    }

    const genetics::Genome& genome = animal->genome;
    const SimConfig& cfg = sim.config();
    const LineageRecord* lineage = sim.lineage().find(animal->id);
    const int generation =
        lineage != nullptr ? lineage->generation : animal->generation;
    const std::uint32_t motherId =
        lineage != nullptr ? lineage->motherId : animal->motherId;
    const std::uint32_t fatherId =
        lineage != nullptr ? lineage->fatherId : animal->fatherId;

    ImGui::Text("Sex: %s", sexName(animal->sex));
    ImGui::Text("Generation: %d", generation);
    ImGui::Text("Age: %.1f s (%s)", animal->age,
                animal->isAdult(cfg) ? "adult" : "juvenile");

    const float maxEnergy = std::max(cfg.maxEnergy, 0.001f);
    char energyText[64];
    std::snprintf(energyText, sizeof(energyText), "%.1f / %.1f",
                  animal->energy, cfg.maxEnergy);
    ImGui::ProgressBar(std::clamp(animal->energy / maxEnergy, 0.f, 1.f),
                       ImVec2(-1.f, 0.f), energyText);

    ImGui::Text("State: %s", stateName(animal->state()).c_str());
    if (animal->sex == genetics::Sex::Female) {
        if (animal->pregnant()) {
            ImGui::Text("pregnant: %d embryo(s), due in %.1f s",
                        static_cast<int>(animal->embryos.size()),
                        animal->gestationTimer);
        } else {
            ImGui::TextUnformatted("not pregnant");
        }
    }

    ImGui::SeparatorText("Genes");
    ImGui::Text("Sight: %.1f  (%.1f | %.1f)", animal->sight, genome.sight[0],
                genome.sight[1]);
    ImGui::Text("Colour: %s",
                phenotypeName(genetics::colourOf(genome)).c_str());
    ImGui::SameLine();
    ImGui::ColorButton("##colourSwatch",
                       panelColour(organismColour(
                           animal->kind(), genetics::colourOf(genome))),
                       ImGuiColorEditFlags_NoTooltip, ImVec2(20.f, 20.f));
    ImGui::Text("Alleles: %s / %s", alleleName(genome.colour[0]).c_str(),
                alleleName(genome.colour[1]).c_str());
    ImGui::TextWrapped("Dark is dominant over Medium and Light.");
    ImGui::Text("Sex chromosomes: %s%s",
                chromosomeName(genome.sexChromosomes[0]).c_str(),
                chromosomeName(genome.sexChromosomes[1]).c_str());

    ImGui::SeparatorText("Family");
    if (motherId == 0 && fatherId == 0) {
        ImGui::TextUnformatted("Parents: Founder");
    } else {
        ImGui::TextUnformatted("Parents:");
        ImGui::SameLine();
        parentButton(sim, motherId, actions);
        ImGui::SameLine();
        ImGui::TextUnformatted("&");
        ImGui::SameLine();
        parentButton(sim, fatherId, actions);
    }
    ImGui::Text("Children born: %d",
                static_cast<int>(sim.lineage().children(animal->id).size()));

    ImGui::End();
}

void drawStatisticsPanel(const Simulation& sim) {
    ImGui::SetNextWindowPos(ImVec2(400.f, kPanelMargin),
                            ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(620.f, 620.f), ImGuiCond_FirstUseEver);
    // Extra left padding leaves room for vertical ImPlot axis labels.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.f, 8.f));
    ImGui::Begin("Statistics");

    const std::vector<StatsSample>& samples = sim.stats().samples();
    const double lastSampleTime =
        samples.empty() ? 0.0 : samples.back().time;
    StatisticsCache& cache = statisticsCache();
    if (cache.sampleCount != samples.size() ||
        cache.lastSampleTime != lastSampleTime) {
        buildChartSeries(sim.stats(), cache.series);
        cache.sampleCount = samples.size();
        cache.lastSampleTime = lastSampleTime;
    }

    const ChartSeries& series = cache.series;
    const int pointCount = static_cast<int>(series.time.size());

    if (ImGui::BeginTabBar("StatisticsTabs")) {
        if (ImGui::BeginTabItem("Population")) {
            if (ImPlot::BeginPlot("Animal population",
                                  ImVec2(-1.f, 220.f))) {
                ImPlot::SetupAxes("seconds", "count",
                                  ImPlotAxisFlags_AutoFit,
                                  ImPlotAxisFlags_AutoFit);
                ImPlot::SetupLegend(ImPlotLocation_NorthWest);
                if (pointCount > 0) {
                    ImPlot::PlotLine("Herbivores", series.time.data(),
                                     series.herbivores.data(), pointCount);
                    ImPlot::PlotLine("Predators", series.time.data(),
                                     series.predators.data(), pointCount);
                }
                ImPlot::EndPlot();
            }
            if (ImPlot::BeginPlot("Plant population",
                                  ImVec2(-1.f, 220.f))) {
                ImPlot::SetupAxes("seconds", "count",
                                  ImPlotAxisFlags_AutoFit,
                                  ImPlotAxisFlags_AutoFit);
                ImPlot::SetupLegend(ImPlotLocation_NorthWest);
                if (pointCount > 0) {
                    ImPlot::PlotLine("Plants", series.time.data(),
                                     series.plants.data(), pointCount);
                }
                ImPlot::EndPlot();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sight")) {
            if (ImPlot::BeginPlot("Mean sight over time",
                                  ImVec2(-1.f, 220.f))) {
                ImPlot::SetupAxes("seconds", "sight",
                                  ImPlotAxisFlags_AutoFit,
                                  ImPlotAxisFlags_AutoFit);
                ImPlot::SetupLegend(ImPlotLocation_NorthWest);
                if (pointCount > 0) {
                    ImPlot::PlotLine("Herbivores", series.time.data(),
                                     series.herbMeanSight.data(), pointCount);
                    ImPlot::PlotLine("Predators", series.time.data(),
                                     series.predMeanSight.data(), pointCount);
                }
                ImPlot::EndPlot();
            }

            const std::vector<float> herbivoreSights =
                livingSights(sim, Kind::Herbivore);
            const std::vector<float> predatorSights =
                livingSights(sim, Kind::Predator);
            if (ImPlot::BeginPlot("Current sight distribution",
                                  ImVec2(-1.f, 250.f))) {
                ImPlot::SetupAxes("sight", "count", 0,
                                  ImPlotAxisFlags_AutoFit);
                ImPlot::SetupAxisLimits(ImAxis_X1, 200.0, 400.0,
                                        ImPlotCond_Always);
                ImPlot::SetupLegend(ImPlotLocation_NorthWest);
                // Semi-transparent bars let both overlapping distributions
                // remain visible in the shared 20-bin histogram.
                ImPlot::PushStyleVar(ImPlotStyleVar_FillAlpha, 0.45f);
                if (!herbivoreSights.empty()) {
                    ImPlot::PlotHistogram(
                        "Herbivores", herbivoreSights.data(),
                        static_cast<int>(herbivoreSights.size()), 20, 1.0,
                        ImPlotRange(200.0, 400.0));
                }
                if (!predatorSights.empty()) {
                    ImPlot::PlotHistogram(
                        "Predators", predatorSights.data(),
                        static_cast<int>(predatorSights.size()), 20, 1.0,
                        ImPlotRange(200.0, 400.0));
                }
                ImPlot::PopStyleVar();
                ImPlot::EndPlot();
            }
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Colour gene")) {
            if (ImPlot::BeginPlot("Dark allele frequency",
                                  ImVec2(-1.f, 320.f))) {
                ImPlot::SetupAxes("seconds", "frequency",
                                  ImPlotAxisFlags_AutoFit, 0);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 1.0,
                                        ImPlotCond_Always);
                ImPlot::SetupLegend(ImPlotLocation_NorthWest);
                if (pointCount > 0) {
                    ImPlot::PlotLine("Herbivores", series.time.data(),
                                     series.herbDarkAlleleFreq.data(),
                                     pointCount);
                    ImPlot::PlotLine("Predators", series.time.data(),
                                     series.predDarkAlleleFreq.data(),
                                     pointCount);
                }
                ImPlot::EndPlot();
            }
            ImGui::TextWrapped(
                "Colour is neutral: changes are random drift, not selection");
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
    ImGui::PopStyleVar();
}
