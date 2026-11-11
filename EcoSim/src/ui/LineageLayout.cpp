// Lineage layout implementation: breadth-first ancestor/descendant collection,
// distance-first pruning, even per-column spacing, and parent-child edges.
#include "ui/LineageLayout.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

LineageLayout buildLineageLayout(const LineageTracker& lineage,
                                 uint32_t centreId, int ancestorGenerations,
                                 int descendantGenerations, int maxNodes) {
    LineageLayout layout;
    if (lineage.find(centreId) == nullptr) {
        return layout;
    }

    std::unordered_map<uint32_t, int> columnOf;
    std::map<int, std::vector<uint32_t>> columns;
    const auto include = [&](uint32_t id, int column) {
        if (columnOf.count(id) != 0) {
            return;
        }
        columnOf.emplace(id, column);
        columns[column].push_back(id);
    };

    include(centreId, 0);

    // Ancestors are visited generation by generation so each node gets the
    // nearest possible negative column.
    std::vector<uint32_t> frontier{centreId};
    for (int depth = 1; depth <= ancestorGenerations && !frontier.empty();
         ++depth) {
        std::vector<uint32_t> next;
        for (uint32_t id : frontier) {
            const LineageRecord* record = lineage.find(id);
            if (record == nullptr) {
                continue;
            }
            const uint32_t parentIds[2] = {record->motherId,
                                           record->fatherId};
            for (uint32_t parentId : parentIds) {
                if (parentId == 0 || lineage.find(parentId) == nullptr ||
                    columnOf.count(parentId) != 0) {
                    continue;
                }
                include(parentId, -depth);
                next.push_back(parentId);
            }
        }
        frontier = std::move(next);
    }

    // Descendants use the tracker's child index so only recorded animals are
    // followed.
    frontier.assign(1, centreId);
    for (int depth = 1; depth <= descendantGenerations && !frontier.empty();
         ++depth) {
        std::vector<uint32_t> next;
        for (uint32_t id : frontier) {
            for (uint32_t childId : lineage.children(id)) {
                if (lineage.find(childId) == nullptr ||
                    columnOf.count(childId) != 0) {
                    continue;
                }
                include(childId, depth);
                next.push_back(childId);
            }
        }
        frontier = std::move(next);
    }

    const std::size_t budget =
        static_cast<std::size_t>(std::max(1, maxNodes));
    std::size_t total = columnOf.size();
    while (total > budget && !columns.empty()) {
        bool removed = false;

        // Drop descendants from the farthest positive column first.
        auto descendant = columns.end();
        --descendant;
        if (descendant->first > 0) {
            descendant->second.pop_back();
            if (descendant->second.empty()) {
                columns.erase(descendant);
            }
            --total;
            removed = true;
        }

        // Only once no descendants remain do we remove the farthest ancestors.
        if (!removed) {
            auto ancestor = columns.begin();
            if (ancestor->first < 0) {
                ancestor->second.pop_back();
                if (ancestor->second.empty()) {
                    columns.erase(ancestor);
                }
                --total;
                removed = true;
            }
        }

        if (!removed) {
            break;  // the centre remains even when maxNodes is zero
        }
    }

    std::unordered_map<uint32_t, int> included;
    for (const auto& entry : columns) {
        for (uint32_t id : entry.second) {
            included.emplace(id, entry.first);
        }
    }

    // Recompute y after pruning so each final column is evenly filled.
    for (auto& entry : columns) {
        std::vector<uint32_t>& ids = entry.second;
        std::sort(ids.begin(), ids.end());
        const float count = static_cast<float>(ids.size());
        for (std::size_t i = 0; i < ids.size(); ++i) {
            layout.nodes.push_back(
                LayoutNode{ids[i], entry.first,
                           (static_cast<float>(i) + 0.5f) / count});
        }
    }

    // Every included parent-to-child pair becomes one straight edge.
    for (const auto& entry : columns) {
        for (uint32_t childId : entry.second) {
            const LineageRecord* child = lineage.find(childId);
            if (child == nullptr) {
                continue;
            }
            const uint32_t parentIds[2] = {child->motherId, child->fatherId};
            for (std::size_t parentIndex = 0; parentIndex < 2; ++parentIndex) {
                const uint32_t parentId = parentIds[parentIndex];
                if (parentId == 0 || included.count(parentId) == 0) {
                    continue;
                }
                if (parentIndex == 1 && parentIds[0] == parentIds[1]) {
                    continue;
                }
                layout.edges.push_back(LayoutEdge{parentId, childId});
            }
        }
    }

    return layout;
}
