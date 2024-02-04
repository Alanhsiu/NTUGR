#include "GlobalRouter.h"
#include "PatternRoute.h"
#include "MazeRoute.h"
#include <chrono>

GlobalRouter::GlobalRouter(const Design& design, const Parameters& params): 
    gridGraph(design, params), parameters(params) {
    // Instantiate the global routing netlist
    const vector<Net>& baseNets = design.netlist.nets;
    nets.reserve(baseNets.size());
    for (const Net& baseNet : baseNets) {
        nets.emplace_back(baseNet, design, gridGraph); // GRNet(baseNet, design, gridGraph)
    }
}

void GlobalRouter::route() {
    // TODO: route each net
    
}

void GlobalRouter::sortNetIndices(vector<int>& netIndices) const {
    // TODO: sort net indices
}

void GlobalRouter::getGuides(const GRNet& net, vector<std::pair<int, utils::BoxT<int>>>& guides) {
    // TODO: get guides for net
}

void GlobalRouter::printStatistics() const {
    // TODO: print statistics
}