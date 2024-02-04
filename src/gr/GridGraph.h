#pragma once
#include "../global.h"
#include "../basic/design.h"
#include "GRTree.h"

using CapacityT = double;
class GRNet;

struct GraphEdge {
    CapacityT capacity;
    CapacityT demand;
    GraphEdge(): capacity(0), demand(0) {}
    CapacityT getResource() const { return capacity - demand; }
};

class GridGraph {
};

