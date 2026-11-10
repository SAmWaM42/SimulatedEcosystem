// Family-tree records for every animal ever born; records survive death so the
// UI and tests can inspect ancestry after organisms are removed.
#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "sim/Organism.h"

// Immutable birth data plus the death stamp for one animal.
struct LineageRecord {
    uint32_t id;
    Kind kind;
    uint32_t motherId;
    uint32_t fatherId;
    int generation;
    double birthTime;
    double deathTime = -1;
    DeathCause deathCause = DeathCause::None;
    genetics::Genome genome;
};

// Retains lineage records forever and indexes children by parent id.
class LineageTracker {
public:
    // Records a live animal and links it into both parents' child lists.
    void recordBirth(const Animal& a, double time);
    // Stamps the stored record with death time and cause; records are kept.
    void recordDeath(const Animal& a, double time);
    // Returns the retained record, or nullptr when the id is unknown.
    const LineageRecord* find(uint32_t id) const;
    // Breadth-first ancestors up to maxGenerations, parents before grandparents.
    std::vector<uint32_t> ancestors(uint32_t id, int maxGenerations) const;
    // Children recorded for this mother or father, or an empty vector.
    const std::vector<uint32_t>& children(uint32_t id) const;
    // Number of retained records.
    std::size_t size() const;

private:
    std::unordered_map<uint32_t, LineageRecord> records;
    std::unordered_map<uint32_t, std::vector<uint32_t>> childrenById;
};
