// SimClock implementation: bounded fixed-step accumulation.
#include "sim/SimClock.h"

#include <algorithm>
#include <cmath>

SimClock::SimClock(float stepDt, int maxSteps)
    : fixedDt(stepDt), maxStepsPerFrame(maxSteps) {}

void SimClock::setSpeed(float s) {
    speedValue = std::clamp(s, 0.25f, 16.f);
}

float SimClock::speed() const {
    return speedValue;
}

void SimClock::setPaused(bool p) {
    pausedValue = p;
}

bool SimClock::paused() const {
    return pausedValue;
}

int SimClock::advance(float frameSeconds) {
    if (pausedValue) {
        return 0;
    }

    accumulator += frameSeconds * speedValue;
    int steps = static_cast<int>(std::floor(accumulator / fixedDt));
    if (steps > maxStepsPerFrame) {
        steps = maxStepsPerFrame;
        // Drop the backlog so a slow frame cannot cascade into a spiral.
        accumulator = 0.f;
    } else {
        accumulator -= static_cast<float>(steps) * fixedDt;
    }
    return steps;
}

float SimClock::alpha() const {
    if (fixedDt <= 0.f) {
        return 0.f;
    }
    return std::clamp(accumulator / fixedDt, 0.f, 0.999999f);
}
