// Fixed-step accumulator used by the interactive application to decouple render
// frame time from simulation steps while keeping a bounded catch-up budget.
#pragma once

// Accumulates time and reports how many fixed simulation steps to run.
class SimClock {
public:
    // fixedDt is the simulation step; maxStepsPerFrame caps catch-up work.
    explicit SimClock(float fixedDt, int maxStepsPerFrame = 20);
    // Sets the time multiplier, clamped to [0.25, 16].
    void setSpeed(float s);
    // Returns the current time multiplier.
    float speed() const;
    // Pauses or resumes advancement without discarding accumulated time.
    void setPaused(bool p);
    // True when paused.
    bool paused() const;
    // Returns the number of fixed steps to run for this frame.
    int advance(float frameSeconds);
    // Returns the leftover fraction of a fixed step in [0, 1).
    float alpha() const;

private:
    float fixedDt;
    int maxStepsPerFrame;
    float accumulator = 0.f;
    float speedValue = 1.f;
    bool pausedValue = false;
};
