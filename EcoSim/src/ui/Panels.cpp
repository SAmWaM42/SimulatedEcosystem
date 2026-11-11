// ImGui/ImPlot panel implementations: controls and settings, HUD, organism
// inspector with lineage links, and the statistics charts.
#include "ui/Panels.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>
#include <unordered_map>
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
#include "ui/LineageLayout.h"
#include "ui/Punnett.h"
#include "ui/UiText.h"

namespace {
constexpr float kPanelMargin = 16.f;
constexpr float kControlsWidth = 370.f;
constexpr float kInspectorTop = 340.f;
constexpr float kLineageSpacing = 130.f;
constexpr float kLineageRadius = 14.f;

// Single-letter allele label used inside compact Punnett table cells.
char alleleLetter(genetics::ColourAllele allele) {
    switch (allele) {
    case genetics::ColourAllele::Dark:
        return 'D';
    case genetics::ColourAllele::Medium:
        return 'M';
    case genetics::ColourAllele::Light:
        return 'L';
    }
    return '?';
}

// Caption shown above each lineage column.
const char* columnCaption(int column) {
    switch (column) {
    case 0:
        return "Selected";
    case 1:
        return "Children";
    case 2:
        return "Grandchildren";
    case -1:
        return "Parents";
    case -2:
        return "Grandparents";
    case -3:
        return "Great-grandparents";
    case -4:
        return "Distant ancestors";
    default:
        return column < 0 ? "Ancestors" : "Descendants";
    }
}

// Letter drawn inside a lineage node.
const char* sexLetter(genetics::Sex sex) {
    return sex == genetics::Sex::Female ? "F" : "M";
}

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
    // A fixed overlay size also renders correctly on the first hidden-window
    // screenshot frame; auto-resize would need a second frame.
    ImGui::SetNextWindowSize(ImVec2(260.f, 220.f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.55f);
    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;

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
    ImGui::Text("Draw calls  %d", hud.drawCalls);
    ImGui::Text("Sim  %.2f ms/frame", hud.simMs);
    ImGui::Text("Render  %.2f ms/frame", hud.renderMs);
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
    ImGui::SetNextWindowSize(ImVec2(600.f, 620.f), ImGuiCond_FirstUseEver);
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
    const double maxTime =
        series.time.empty() ? 1.0 : std::max(1.0, series.time.back());
    double maxAnimals = 0.0;
    double maxPlants = 0.0;
    for (std::size_t i = 0; i < series.time.size(); ++i) {
        maxAnimals = std::max(
            maxAnimals, std::max(series.herbivores[i],
                                 series.predators[i]));
        maxPlants = std::max(maxPlants, series.plants[i]);
    }
    maxAnimals = std::max(1.0, maxAnimals * 1.08);
    maxPlants = std::max(1.0, maxPlants * 1.08);

    if (ImGui::BeginTabBar("StatisticsTabs")) {
        if (ImGui::BeginTabItem("Population")) {
            if (ImPlot::BeginPlot("Animal population",
                                  ImVec2(-1.f, 220.f))) {
                ImPlot::SetupAxes("seconds", "count");
                ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, maxTime,
                                        ImPlotCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, maxAnimals,
                                        ImPlotCond_Always);
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
                ImPlot::SetupAxes("seconds", "count");
                ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, maxTime,
                                        ImPlotCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, maxPlants,
                                        ImPlotCond_Always);
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
                ImPlot::SetupAxes("seconds", "sight");
                ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, maxTime,
                                        ImPlotCond_Always);
                ImPlot::SetupAxisLimits(ImAxis_Y1, 200.0, 400.0,
                                        ImPlotCond_Always);
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
                ImPlot::SetupAxes("seconds", "frequency");
                ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, maxTime,
                                        ImPlotCond_Always);
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

void drawLineagePanel(const Simulation& sim, std::uint32_t selectedId,
                      UiActions& actions) {
    const Organism* selected = sim.find(selectedId);
    if (selected == nullptr || selected->kind() == Kind::Plant) {
        return;
    }
    const LineageRecord* centreRecord = sim.lineage().find(selectedId);
    if (centreRecord == nullptr) {
        return;
    }

    const LineageLayout layout = buildLineageLayout(sim.lineage(), selectedId);
    if (layout.nodes.empty()) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(400.f, 340.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(620.f, 350.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Lineage");

    int minColumn = layout.nodes.front().column;
    int maxColumn = minColumn;
    std::unordered_map<int, int> perColumn;
    for (const LayoutNode& node : layout.nodes) {
        minColumn = std::min(minColumn, node.column);
        maxColumn = std::max(maxColumn, node.column);
        ++perColumn[node.column];
    }
    int maxCount = 1;
    for (const auto& entry : perColumn) {
        maxCount = std::max(maxCount, entry.second);
    }

    const int columnCount = maxColumn - minColumn + 1;
    const float canvasWidth = std::max(
        ImGui::GetContentRegionAvail().x,
        static_cast<float>(columnCount - 1) * kLineageSpacing + 160.f);
    const float canvasHeight =
        std::max(240.f, 84.f + 34.f * static_cast<float>(maxCount));

    if (ImGui::BeginChild("##lineageCanvas", ImVec2(0.f, 0.f), true,
                          ImGuiWindowFlags_HorizontalScrollbar)) {
        const ImVec2 origin = ImGui::GetCursorScreenPos();
        ImGui::Dummy(ImVec2(canvasWidth, canvasHeight));
        ImDrawList* drawList = ImGui::GetWindowDrawList();

        const float nodeTop = 50.f;
        const float nodeBottom = canvasHeight - 28.f;
        std::unordered_map<uint32_t, ImVec2> centres;

        for (int column = minColumn; column <= maxColumn; ++column) {
            const char* caption = columnCaption(column);
            const ImVec2 captionSize = ImGui::CalcTextSize(caption);
            const float centreX =
                origin.x + 80.f +
                static_cast<float>(column - minColumn) * kLineageSpacing;
            drawList->AddText(
                ImVec2(centreX - captionSize.x * 0.5f, origin.y + 8.f),
                ImGui::GetColorU32(ImGuiCol_TextDisabled), caption);
        }

        for (const LayoutNode& node : layout.nodes) {
            const float x =
                origin.x + 80.f +
                static_cast<float>(node.column - minColumn) * kLineageSpacing;
            const float y =
                origin.y + nodeTop + node.y * (nodeBottom - nodeTop);
            centres.emplace(node.id, ImVec2(x, y));
        }

        for (const LayoutEdge& edge : layout.edges) {
            const auto parent = centres.find(edge.parentId);
            const auto child = centres.find(edge.childId);
            if (parent != centres.end() && child != centres.end()) {
                drawList->AddLine(parent->second, child->second,
                                  IM_COL32(145, 145, 145, 190), 1.5f);
            }
        }

        const ImVec2 mouse = ImGui::GetIO().MousePos;
        const bool canvasHovered = ImGui::IsWindowHovered();
        uint32_t hoveredId = 0;

        for (const LayoutNode& node : layout.nodes) {
            const LineageRecord* record = sim.lineage().find(node.id);
            if (record == nullptr) {
                continue;
            }
            const auto centreIt = centres.find(node.id);
            if (centreIt == centres.end()) {
                continue;
            }
            const ImVec2 centre = centreIt->second;
            const bool alive = sim.find(node.id) != nullptr;
            glm::vec3 fillColour = organismColour(
                record->kind, genetics::colourOf(record->genome));
            if (!alive) {
                fillColour *= 0.45f;
            }
            const ImU32 fill = ImGui::ColorConvertFloat4ToU32(
                ImVec4(fillColour.r, fillColour.g, fillColour.b, 1.f));
            const ImU32 outline =
                alive ? IM_COL32(245, 245, 245, 255)
                      : IM_COL32(145, 145, 145, 255);

            drawList->AddCircleFilled(centre, kLineageRadius, fill, 32);
            drawList->AddCircle(centre, kLineageRadius, outline, 32, 2.f);
            if (node.column == 0) {
                drawList->AddCircle(centre, kLineageRadius + 5.f,
                                    IM_COL32(255, 215, 0, 255), 32, 3.f);
            }

            const char* letter =
                sexLetter(genetics::sexOf(record->genome));
            const ImVec2 letterSize = ImGui::CalcTextSize(letter);
            const float luminance = 0.2126f * fillColour.r +
                                    0.7152f * fillColour.g +
                                    0.0722f * fillColour.b;
            const ImU32 letterColour =
                luminance > 0.5f ? IM_COL32(20, 20, 20, 255)
                                 : IM_COL32(255, 255, 255, 255);
            drawList->AddText(
                ImGui::GetFont(), ImGui::GetFontSize(),
                ImVec2(centre.x - letterSize.x * 0.5f,
                       centre.y - letterSize.y * 0.5f),
                letterColour, letter);

            const float dx = mouse.x - centre.x;
            const float dy = mouse.y - centre.y;
            const float hitRadius = kLineageRadius + 2.f;
            if (canvasHovered && dx * dx + dy * dy <= hitRadius * hitRadius) {
                hoveredId = node.id;
            }
        }

        if (hoveredId != 0) {
            const LineageRecord* record = sim.lineage().find(hoveredId);
            if (record != nullptr && ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
                sim.find(hoveredId) != nullptr) {
                actions.selectId = hoveredId;
            }

            if (record != nullptr) {
                ImGui::BeginTooltip();
                ImGui::Text("%s #%u", kindName(record->kind).c_str(),
                            static_cast<unsigned int>(record->id));
                ImGui::Text("Sex: %s   Generation: %d",
                            sexName(genetics::sexOf(record->genome)),
                            record->generation);
                ImGui::Text("Sight: %.1f   Colour: %s",
                            genetics::sightOf(record->genome),
                            phenotypeName(genetics::colourOf(record->genome))
                                .c_str());
                ImGui::Text("Born: %.1f s", record->birthTime);
                if (record->deathTime >= 0.0) {
                    ImGui::Text("Died: %.1f s (%s)", record->deathTime,
                                deathCauseName(record->deathCause).c_str());
                } else {
                    ImGui::TextUnformatted("alive");
                }
                ImGui::EndTooltip();
            }
        }

        ImGui::EndChild();
    }

    ImGui::End();
}

void drawGeneticsPanel(const Simulation& sim, std::uint32_t selectedId) {
    const Organism* selected = sim.find(selectedId);
    if (selected == nullptr || selected->kind() == Kind::Plant) {
        return;
    }
    const LineageRecord* child = sim.lineage().find(selectedId);
    if (child == nullptr) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(1030.f, 260.f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(234.f, 390.f), ImGuiCond_FirstUseEver);
    ImGui::Begin("Genetics of this animal");

    if (child->motherId == 0 && child->fatherId == 0) {
        ImGui::TextWrapped("Founder: no recorded parents");
        ImGui::End();
        return;
    }

    const LineageRecord* mother = sim.lineage().find(child->motherId);
    const LineageRecord* father = sim.lineage().find(child->fatherId);
    const PunnettInfo info = buildPunnett(child, mother, father);
    if (!info.available) {
        ImGui::TextWrapped("Parent records unavailable.");
        ImGui::End();
        return;
    }

    ImGui::TextWrapped("Mother alleles (rows) x father alleles (columns).");
    const ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
        ImGuiTableFlags_SizingStretchSame;
    if (ImGui::BeginTable("##punnett", 3, tableFlags)) {
        ImGui::TableSetupColumn("##corner");
        ImGui::TableSetupColumn("##father0");
        ImGui::TableSetupColumn("##father1");

        ImGui::TableNextRow(ImGuiTableRowFlags_Headers);
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted("M / F");
        for (int column = 0; column < 2; ++column) {
            ImGui::TableSetColumnIndex(column + 1);
            ImGui::TextUnformatted(
                alleleName(father->genome.colour[column]).c_str());
        }

        for (int row = 0; row < 2; ++row) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextUnformatted(
                alleleName(mother->genome.colour[row]).c_str());
            for (int column = 0; column < 2; ++column) {
                ImGui::TableSetColumnIndex(column + 1);
                const PunnettCell& cell = info.cells[row][column];
                if (cell.isChild) {
                    ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg,
                                           IM_COL32(62, 125, 72, 170));
                }
                ImGui::PushID(row * 2 + column);
                ImGui::ColorButton(
                    "##phenotype",
                    panelColour(organismColour(child->kind, cell.phenotype)),
                    ImGuiColorEditFlags_NoTooltip, ImVec2(16.f, 16.f));
                ImGui::SameLine(0.f, 4.f);
                ImGui::Text("%c/%c", alleleLetter(cell.fromMother),
                            alleleLetter(cell.fromFather));
                ImGui::PopID();
            }
        }
        ImGui::EndTable();
    }
    ImGui::TextDisabled("D=Dark  M=Medium  L=Light");

    char sightLine[512];
    std::snprintf(
        sightLine, sizeof(sightLine),
        "Sight: mother (%.1f|%.1f) x father (%.1f|%.1f) -> "
        "child (%.1f|%.1f), expressed %.1f",
        mother->genome.sight[0], mother->genome.sight[1],
        father->genome.sight[0], father->genome.sight[1],
        child->genome.sight[0], child->genome.sight[1],
        genetics::sightOf(child->genome));
    ImGui::TextWrapped("%s", sightLine);

    const ImVec4 warning(1.f, 0.78f, 0.25f, 1.f);
    ImGui::PushStyleColor(ImGuiCol_Text, warning);
    if (info.sightMutationMother) {
        ImGui::TextWrapped(
            "Mutation detected in the maternal sight allele");
    }
    if (info.sightMutationFather) {
        ImGui::TextWrapped(
            "Mutation detected in the paternal sight allele");
    }
    if (info.colourMutationMother) {
        ImGui::TextWrapped(
            "Mutation detected in the maternal colour allele");
    }
    if (info.colourMutationFather) {
        ImGui::TextWrapped(
            "Mutation detected in the paternal colour allele");
    }
    ImGui::PopStyleColor();

    ImGui::End();
}
