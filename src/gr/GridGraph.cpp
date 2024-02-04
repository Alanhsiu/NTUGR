#include "GridGraph.h"
#include "GRNet.h"

GridGraph::GridGraph(const Design& design, const Parameters& params)
    : parameters(params) {
    nLayers = design.dimension.n_layers;
    xSize = design.dimension.x_size;
    ySize = design.dimension.y_size;

    // initialize gridlines using hEdge and vEdge
    gridlines.resize(2);
    gridlines[0].resize(xSize + 1);
    gridlines[1].resize(ySize + 1);
    gridlines[0][0] = 0;
    gridlines[1][0] = 0;
    for (unsigned i = 1; i <= xSize; i++)
        gridlines[0][i] = gridlines[0][i - 1] + design.dimension.hEdge[i - 1];
    for (unsigned i = 1; i <= ySize; i++)
        gridlines[1][i] = gridlines[1][i - 1] + design.dimension.vEdge[i - 1];

    // initialize gridCenters using gridlines
    gridCenters.resize(2);
    for (unsigned i = 0; i < 2; i++) {
        gridCenters[i].resize(gridlines[i].size() - 1);
        for (unsigned j = 0; j < gridCenters[i].size(); j++)
            gridCenters[i][j] = (gridlines[i][j] + gridlines[i][j + 1]) / 2;
    }

    // initialize layerDirections and layerMinLengths
    layerDirections.resize(nLayers);
    layerMinLengths.resize(nLayers);
    for (unsigned i = 0; i < nLayers; i++) {
        layerDirections[i] = design.layers[i].direction;
        layerMinLengths[i] = design.layers[i].minLength;
    }

    // initialize cost parameters
    UnitLengthWireCost = design.metrics.UnitLengthWireCost;
    UnitViaCost = design.metrics.UnitViaCost;
    OFWeight = design.metrics.OFWeight;

    // initialize graphEdges using capacity
    graphEdges.assign(nLayers, vector<vector<GraphEdge>>(xSize, vector<GraphEdge>(ySize)));
    for (unsigned l = 0; l < nLayers; l++) {
        for (unsigned x = 0; x < xSize; x++) {
            for (unsigned y = 0; y < ySize; y++) {
                graphEdges[l][x][y].capacity = design.layers[l].capacity[x][y];
            }
        }
    }
}

utils::IntervalT<int> GridGraph::rangeSearchGridlines(const unsigned dimension, const utils::IntervalT<DBU>& locInterval) const {
    utils::IntervalT<int> range;
    range.low = lower_bound(gridlines[dimension].begin(), gridlines[dimension].end(), locInterval.low) - gridlines[dimension].begin();
    range.high = lower_bound(gridlines[dimension].begin(), gridlines[dimension].end(), locInterval.high) - gridlines[dimension].begin();
    if (range.high >= gridlines[dimension].size()) {
        range.high = gridlines[dimension].size() - 1;
    } else if (gridlines[dimension][range.high] > locInterval.high) {
        range.high -= 1;
    }
    return range;
}
  
utils::IntervalT<int> GridGraph::rangeSearchRows(const unsigned dimension, const utils::IntervalT<DBU>& locInterval) const {
    const auto& lineRange = rangeSearchGridlines(dimension, locInterval);
    return {
        gridlines[dimension][lineRange.low] == locInterval.low ? lineRange.low : max(lineRange.low - 1, 0),
        gridlines[dimension][lineRange.high] == locInterval.high ? lineRange.high - 1 : min(lineRange.high, static_cast<int>(getSize(dimension)) - 1)
    };
}

utils::BoxT<DBU> GridGraph::getCellBox(utils::PointT<int> point) const {
    return {
        getGridline(0, point.x), getGridline(1, point.y),
        getGridline(0, point.x + 1), getGridline(1, point.y + 1)
    };
}

utils::BoxT<int> GridGraph::rangeSearchCells(const utils::BoxT<DBU>& box) const {
    return {rangeSearchRows(0, box[0]), rangeSearchRows(1, box[1])};
}

DBU GridGraph::getEdgeLength(unsigned direction, unsigned edgeIndex) const {
    return gridlines[direction][edgeIndex + 1] - gridlines[direction][edgeIndex];
}

inline double GridGraph::logistic(const double& input, const double slope) const {
    return 1.0 / (1.0 + exp(input * slope));
}

double GridGraph::getWireCost(const int layerIndex, const utils::PointT<int> lower, const double demand) const {
    unsigned direction = layerDirections[layerIndex];
    DBU edgeLength = getEdgeLength(direction, lower[direction]);
    DBU demandLength = demand * edgeLength;
    const auto& edge = graphEdges[layerIndex][lower.x][lower.y];
    double cost = demandLength * UnitLengthWireCost;
    cost += demandLength * (
        edge.capacity < 1.0 ? 1.0 : logistic(edge.capacity - edge.demand, parameters.cost_logistic_slope)
    );
    return cost;
}

double_t GridGraph::getViaCost(const int layerIndex, const utils::PointT<int> loc) const {
    assert(layerIndex + 1 < nLayers);
    double cost = UnitViaCost;
    // Estimated wire cost to satisfy min-area
    for (int l = layerIndex; l <= layerIndex + 1; l++) {
        unsigned direction = layerDirections[l];
        utils::PointT<int> lowerLoc = loc;
        lowerLoc[direction] -= 1;
        DBU lowerEdgeLength = loc[direction] > 0 ? getEdgeLength(direction, lowerLoc[direction]) : 0;
        DBU higherEdgeLength = loc[direction] < getSize(direction) - 1 ? getEdgeLength(direction, loc[direction]) : 0;
        double demand = (double) layerMinLengths[l] / (lowerEdgeLength + higherEdgeLength) * parameters.via_multiplier;
        if (lowerEdgeLength > 0) cost += getWireCost(l, lowerLoc, demand);
        if (higherEdgeLength > 0) cost += getWireCost(l, loc, demand);
    }
    return cost;
}

void GridGraph::selectAccessPoints(GRNet& net, robin_hood::unordered_map<uint64_t, std::pair<utils::PointT<int>, utils::IntervalT<int>>>& selectedAccessPoints) const {
    selectedAccessPoints.clear();
    // cell hash (2d) -> access point, fixed layer interval
    selectedAccessPoints.reserve(net.getNumPins());
    const auto& boundingBox = net.getBoundingBox();
    utils::PointT<int> netCenter(boundingBox.cx(), boundingBox.cy());
    for (const auto& accessPoints : net.getPinAccessPoints()) {
        std::pair<int, int> bestAccessDist = {0, std::numeric_limits<int>::max()};
        int bestIndex = -1;
        for (int index = 0; index < accessPoints.size(); index++) {
            const auto& point = accessPoints[index];
            int accessibility = 0;
            if (point.layerIdx >= 1) {
                unsigned direction = getLayerDirection(point.layerIdx);
                accessibility += getEdge(point.layerIdx, point.x, point.y).capacity >= 1;
                if (point[direction] > 0) {
                    auto lower = point;
                    lower[direction] -= 1;
                    accessibility += getEdge(lower.layerIdx, lower.x, lower.y).capacity >= 1;
                }
            } else {
                accessibility = 1;
            }
            int distance = abs(netCenter.x - point.x) + abs(netCenter.y - point.y);
            if (accessibility > bestAccessDist.first || (accessibility == bestAccessDist.first && distance < bestAccessDist.second)) {
                bestIndex = index;
                bestAccessDist = {accessibility, distance};
            }
        }
        if (bestAccessDist.first == 0) {
            log() << "Warning: the pin is hard to access." << std::endl;
        }
        const utils::PointT<int> selectedPoint = accessPoints[bestIndex];
        const uint64_t hash = hashCell(selectedPoint.x, selectedPoint.y);
        if (selectedAccessPoints.find(hash) == selectedAccessPoints.end()) {
            selectedAccessPoints.emplace(hash, std::make_pair(selectedPoint, utils::IntervalT<int>()));
        }
        utils::IntervalT<int>& fixedLayerInterval = selectedAccessPoints[hash].second;
        for (const auto& point : accessPoints) {
            if (point.x == selectedPoint.x && point.y == selectedPoint.y) {
                fixedLayerInterval.Update(point.layerIdx);
            }
        }
    }
    // Extend the fixed layers to 2 layers higher to facilitate track switching
    for (auto& accessPoint : selectedAccessPoints) {
        utils::IntervalT<int>& fixedLayers = accessPoint.second.second;
        fixedLayers.high = min(fixedLayers.high + 2, (int)getNumLayers() - 1);
    }
}

void GridGraph::commit(const int layerIndex, const utils::PointT<int> lower, const double demand) {
    graphEdges[layerIndex][lower.x][lower.y].demand += demand;
}

void GridGraph::commitWire(const int layerIndex, const utils::PointT<int> lower, const bool reverse) {
    unsigned direction = layerDirections[layerIndex];
    DBU edgeLength = getEdgeLength(direction, lower[direction]);
    if (reverse) {
        commit(layerIndex, lower, -1);
        totalLength -= edgeLength;
    } else {
        commit(layerIndex, lower, 1);
        totalLength += edgeLength;
    }
}

void GridGraph::commitVia(const int layerIndex, const utils::PointT<int> loc, const bool reverse) {
    assert(layerIndex + 1 < nLayers);
    for (int l = layerIndex; l <= layerIndex + 1; l++) {
        unsigned direction = layerDirections[l];
        utils::PointT<int> lowerLoc = loc;
        lowerLoc[direction] -= 1;
        DBU lowerEdgeLength = loc[direction] > 0 ? getEdgeLength(direction, lowerLoc[direction]) : 0;
        DBU higherEdgeLength = loc[direction] < getSize(direction) - 1 ? getEdgeLength(direction, loc[direction]) : 0;
        double demand = (double) layerMinLengths[l] / (lowerEdgeLength + higherEdgeLength) * parameters.via_multiplier;
        if (lowerEdgeLength > 0) commit(l, lowerLoc, (reverse ? -demand : demand));
        if (higherEdgeLength > 0) commit(l, loc, (reverse ? -demand : demand));
    }
    if (reverse) totalNumVias -= 1;
    else totalNumVias += 1;
}

void GridGraph::commitTree(const std::shared_ptr<GRTreeNode>& tree, const bool reverse) {
    GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
        for (const auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                unsigned direction = layerDirections[node->layerIdx];
                if (direction == 0) {
                    assert(node->y == child->y);
                    int l = min(node->x, child->x), h = max(node->x, child->x);
                    for (int x = l; x < h; x++) {
                        commitWire(node->layerIdx, {x, node->y}, reverse);
                    }
                } else {
                    assert(node->x == child->x);
                    int l = min(node->y, child->y), h = max(node->y, child->y);
                    for (int y = l; y < h; y++) {
                        commitWire(node->layerIdx, {node->x, y}, reverse);
                    }
                }
            } else {
                int maxLayerIndex = max(node->layerIdx, child->layerIdx);
                for (int layerIdx = min(node->layerIdx, child->layerIdx); layerIdx < maxLayerIndex; layerIdx++) {
                    commitVia(layerIdx, {node->x, node->y}, reverse);
                }
            }
        }
    });
}


int GridGraph::checkOverflow(const int layerIndex, const utils::PointT<int> u, const utils::PointT<int> v) const {
    int num = 0;
    unsigned direction = layerDirections[layerIndex];
    if (direction == 0) {
        assert(u.y == v.y);
        int l = min(u.x, v.x), h = max(u.x, v.x);
        for (int x = l; x < h; x++) {
            if (checkOverflow(layerIndex, x, u.y)) num++;
        }
    } else {
        assert(u.x == v.x);
        int l = min(u.y, v.y), h = max(u.y, v.y);
        for (int y = l; y < h; y++) {
            if (checkOverflow(layerIndex, u.x, y)) num++;
        }
    }
    return num;
}

int GridGraph::checkOverflow(const std::shared_ptr<GRTreeNode>& tree) const {
    if (!tree) return 0;
    int num = 0;
    GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
        for (auto& child : node->children) {
            // Only check wires
            if (node->layerIdx == child->layerIdx) {
                num += checkOverflow(node->layerIdx, (utils::PointT<int>)*node, (utils::PointT<int>)*child);
            }
        }
    });
    return num;
}


std::string GridGraph::getPythonString(const std::shared_ptr<GRTreeNode>& routingTree) const {
    vector<std::tuple<utils::PointT<int>, utils::PointT<int>, bool>> edges;
    GRTreeNode::preorder(routingTree, [&] (std::shared_ptr<GRTreeNode> node) {
        for (auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                unsigned direction = getLayerDirection(node->layerIdx);
                int r = (*node)[1 - direction];
                const int l = min((*node)[direction], (*child)[direction]);
                const int h = max((*node)[direction], (*child)[direction]);
                if (l == h) continue;
                utils::PointT<int> lpoint = (direction == 0 ? utils::PointT<int>(l, r) : utils::PointT<int>(r, l));
                utils::PointT<int> hpoint = (direction == 0 ? utils::PointT<int>(h, r) : utils::PointT<int>(r, h));
                bool congested = false;
                for (int c = l; c < h; c++) {
                    utils::PointT<int> cpoint = (direction == 0 ? utils::PointT<int>(c, r) : utils::PointT<int>(r, c));
                    if (checkOverflow(node->layerIdx, cpoint.x, cpoint.y) != congested) {
                        if (lpoint != cpoint) {
                            edges.emplace_back(lpoint, cpoint, congested);
                            lpoint = cpoint;
                        }
                        congested = !congested;
                    } 
                }
                if (lpoint != hpoint) edges.emplace_back(lpoint, hpoint, congested);
            }
        }
    });
    std::stringstream ss;
    ss << "[";
    for (int i = 0; i < edges.size(); i++) {
        auto& edge = edges[i];
        ss << "[" << std::get<0>(edge) << ", " << std::get<1>(edge) << ", " << (std::get<2>(edge) ? 1 : 0) << "]";
        ss << (i < edges.size() - 1 ? ", " : "]");
    }
    return ss.str();
}
