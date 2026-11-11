// Immediate-mode UI panels for the simulation. Panel functions take current
// state and return user actions; only ImGui/ImPlot code lives in the .cpp.
#pragma once

#include <cstdint>

struct SimConfig;
class SimClock;
class Simulation;

// User choices collected from all panels during one frame.
struct UiActions {
    bool togglePause = false;
    bool restart = false;
    bool newSeed = false;
    bool toggleFollow = false;
    std::uint32_t selectId = 0;
};

// Values shown by the compact HUD overlay.
struct HudData {
    double time = 0.0;
    int generation = 0;
    int plants = 0;
    int herbivoreFemales = 0;
    int herbivoreMales = 0;
    int predatorFemales = 0;
    int predatorMales = 0;
    int fps = 0;
    bool paused = false;
};

// Draws the controls window, including the collapsible settings section.
void drawControlsPanel(SimClock& clock, bool followSelected, bool hasSelection,
                       bool& showSightRing, SimConfig& runConfig,
                       Simulation& sim, std::uint32_t seed, UiActions& actions);

// Draws the translucent top-right status overlay.
void drawHud(const HudData& hud);

// Draws details for the selected organism and its lineage.
void drawInspectorPanel(const Simulation& sim, std::uint32_t selectedId,
                        UiActions& actions);

// Draws the ImPlot statistics window with its tab bar.
void drawStatisticsPanel(const Simulation& sim);
