// GL-free family-tree layout: picks a bounded set of ancestors/descendants and
// assigns columns and evenly spread vertical positions for the lineage view.
#pragma once

#include <cstdint>
#include <vector>

#include "sim/Lineage.h"

// One circle in the lineage canvas.
struct LayoutNode {
    uint32_t id;
    int column;  // 0 = selected, negative = ancestors, positive = descendants
    float y;     // vertical position in [0, 1]
};

// One parent-to-child link between included nodes.
struct LayoutEdge {
    uint32_t parentId;
    uint32_t childId;
};

// Nodes and edges ready to draw; empty when the centre is unknown.
struct LineageLayout {
    std::vector<LayoutNode> nodes;
    std::vector<LayoutEdge> edges;
};

// Builds a bounded tree around centreId. Farthest descendants are dropped
// before farthest ancestors when the node budget is exceeded.
LineageLayout buildLineageLayout(const LineageTracker& lineage,
                                 uint32_t centreId,
                                 int ancestorGenerations = 4,
                                 int descendantGenerations = 2,
                                 int maxNodes = 60);
