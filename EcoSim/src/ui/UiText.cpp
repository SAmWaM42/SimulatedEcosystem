// GL-free text formatting for enum values and simulated time.
#include "ui/UiText.h"

#include <cmath>
#include <iomanip>
#include <sstream>

std::string kindName(Kind kind) {
    switch (kind) {
    case Kind::Plant:
        return "Plant";
    case Kind::Herbivore:
        return "Herbivore";
    case Kind::Predator:
        return "Predator";
    }
    return "Unknown";
}

std::string stateName(State state) {
    switch (state) {
    case State::Wander:
        return "Wander";
    case State::SeekFood:
        return "SeekFood";
    case State::Flee:
        return "Flee";
    case State::SeekMate:
        return "SeekMate";
    }
    return "Unknown";
}

std::string phenotypeName(genetics::ColourPhenotype phenotype) {
    switch (phenotype) {
    case genetics::ColourPhenotype::Dark:
        return "Dark";
    case genetics::ColourPhenotype::Medium:
        return "Medium";
    case genetics::ColourPhenotype::Light:
        return "Light";
    }
    return "Unknown";
}

std::string alleleName(genetics::ColourAllele allele) {
    switch (allele) {
    case genetics::ColourAllele::Dark:
        return "Dark";
    case genetics::ColourAllele::Medium:
        return "Medium";
    case genetics::ColourAllele::Light:
        return "Light";
    }
    return "Unknown";
}

std::string chromosomeName(genetics::SexChromosome chromosome) {
    return chromosome == genetics::SexChromosome::X ? "X" : "Y";
}

std::string deathCauseName(DeathCause cause) {
    switch (cause) {
    case DeathCause::None:
        return "None";
    case DeathCause::Starved:
        return "Starved";
    case DeathCause::OldAge:
        return "Old age";
    case DeathCause::Eaten:
        return "Eaten";
    }
    return "Unknown";
}

std::string formatClock(double seconds) {
    // Treat negative and non-finite values as the start of the simulation.
    const double safeSeconds =
        std::isfinite(seconds) && seconds > 0.0 ? seconds : 0.0;
    const long long totalSeconds = static_cast<long long>(safeSeconds);
    const long long minutes = totalSeconds / 60;
    const long long remainingSeconds = totalSeconds % 60;

    std::ostringstream out;
    out << std::setfill('0') << std::setw(2) << minutes << ':'
        << std::setw(2) << remainingSeconds;
    return out.str();
}
