// Lineage tracker implementation: retained records, parent links, breadth-first
// ancestor walks, and death stamping.
#include "sim/Lineage.h"

#include <unordered_set>
#include <utility>

void LineageTracker::recordBirth(const Animal& a, double time) {
    LineageRecord record;
    record.id = a.id;
    record.kind = a.kind();
    record.motherId = a.motherId;
    record.fatherId = a.fatherId;
    record.generation = a.generation;
    record.birthTime = time;
    record.deathTime = -1.0;
    record.deathCause = DeathCause::None;
    record.genome = a.genome;

    const bool inserted = records.emplace(a.id, record).second;
    if (!inserted) {
        return;
    }

    if (a.motherId != 0) {
        childrenById[a.motherId].push_back(a.id);
    }
    if (a.fatherId != 0 && a.fatherId != a.motherId) {
        childrenById[a.fatherId].push_back(a.id);
    }
}

void LineageTracker::recordDeath(const Animal& a, double time) {
    const auto entry = records.find(a.id);
    if (entry == records.end() || entry->second.deathTime >= 0.0) {
        return;
    }
    entry->second.deathTime = time;
    entry->second.deathCause = a.deathCause;
}

const LineageRecord* LineageTracker::find(uint32_t id) const {
    const auto entry = records.find(id);
    return entry == records.end() ? nullptr : &entry->second;
}

std::vector<uint32_t> LineageTracker::ancestors(uint32_t id,
                                                int maxGenerations) const {
    std::vector<uint32_t> result;
    if (maxGenerations <= 0) {
        return result;
    }

    std::unordered_set<uint32_t> visited;
    visited.insert(id);
    std::vector<uint32_t> currentLevel{id};

    for (int generation = 0; generation < maxGenerations; ++generation) {
        std::vector<uint32_t> nextLevel;
        for (uint32_t currentId : currentLevel) {
            const auto entry = records.find(currentId);
            if (entry == records.end()) {
                continue;
            }

            const uint32_t parents[2] = {entry->second.motherId,
                                         entry->second.fatherId};
            for (uint32_t parentId : parents) {
                if (parentId == 0 || visited.count(parentId) != 0) {
                    continue;
                }
                visited.insert(parentId);
                result.push_back(parentId);
                nextLevel.push_back(parentId);
            }
        }

        currentLevel = std::move(nextLevel);
        if (currentLevel.empty()) {
            break;
        }
    }

    return result;
}

const std::vector<uint32_t>& LineageTracker::children(uint32_t id) const {
    const auto entry = childrenById.find(id);
    if (entry == childrenById.end()) {
        static const std::vector<uint32_t> empty;
        return empty;
    }
    return entry->second;
}

std::size_t LineageTracker::size() const {
    return records.size();
}
