#include "GridGraph.h"
#include "GRNet.h"

GridGraph::GridGraph(const Design& design, const Parameters& params)
    : parameters(params) {
    nLayers = design.dimension.n_layers;
    xSize = design.dimension.x_size;
    ySize = design.dimension.y_size;

    hEdge = design.dimension.hEdge;
    vEdge = design.dimension.vEdge;  // for getEdgeLength()

    // initialize gridCenters hEdge and vEdge
    gridCenters.resize(2);
    gridCenters[0].resize(xSize);
    gridCenters[1].resize(ySize);
    gridCenters[0][0] = 0;
    gridCenters[1][0] = 0;
    for (unsigned i = 1; i < xSize; i++) {  // accumulate
        gridCenters[0][i] = gridCenters[0][i - 1] + design.dimension.hEdge[i - 1];
    }
    for (unsigned i = 1; i < ySize; i++) {  // accumulate
        gridCenters[1][i] = gridCenters[1][i - 1] + design.dimension.vEdge[i - 1];
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
                graphEdges[l][x][y].capacity = design.layers[l].capacity[y][x];  // Note capacity is stored in reverse order
            }
        }
    }
}

DBU GridGraph::getEdgeLength(unsigned direction, unsigned edgeIndex) const {
    return direction == 0 ? hEdge[edgeIndex] : vEdge[edgeIndex];
    // return gridCenters[direction][edgeIndex];
    // return gridlines[direction][edgeIndex + 1] - gridlines[direction][edgeIndex];
}

inline double GridGraph::logistic(const CapacityT& input, bool s) const {
    double slope = s ? parameters.cost_logistic_slope1 : parameters.cost_logistic_slope2;
    return exp(-input * slope);
}

CostT GridGraph::getWireCost(const int layerIndex, const utils::PointT<int> lower, const CapacityT demand) const {
    unsigned direction = layerDirections[layerIndex];
    DBU edgeLength = getEdgeLength(direction, lower[direction]);
    DBU demandLength = demand * edgeLength;
    const auto& edge = graphEdges[layerIndex][lower.x][lower.y];
    CostT cost = demandLength * UnitLengthWireCost;
    bool s = edge.capacity < 0.01;
    cost += demandLength * logistic(edge.capacity - edge.demand, s);
    return cost;
}

CostT GridGraph::getWireCost(const int layerIndex, const utils::PointT<int> u, const utils::PointT<int> v) const {
    unsigned direction = layerDirections[layerIndex];
    assert(u[1 - direction] == v[1 - direction]);
    CostT cost = 0;
    if (direction == 0) {
        int l = min(u.x, v.x), h = max(u.x, v.x);
        for (int x = l; x < h; x++)
            cost += getWireCost(layerIndex, {x, u.y});
    } else {
        int l = min(u.y, v.y), h = max(u.y, v.y);
        for (int y = l; y < h; y++)
            cost += getWireCost(layerIndex, {u.x, y});
    }
    return cost;
}

CostT GridGraph::getViaCost(const int layerIndex, const utils::PointT<int> loc) const {
    assert(layerIndex + 1 < nLayers);
    CostT cost = UnitViaCost;
    // Estimated wire cost to satisfy min-area
    for (int l = layerIndex; l <= layerIndex + 1; l++) {
        unsigned direction = layerDirections[l];
        utils::PointT<int> lowerLoc = loc;
        lowerLoc[direction] -= 1;
        DBU lowerEdgeLength = loc[direction] > 0 ? getEdgeLength(direction, lowerLoc[direction]) : 0;
        DBU higherEdgeLength = loc[direction] < getSize(direction) - 1 ? getEdgeLength(direction, loc[direction]) : 0;
        // CapacityT demand = (CapacityT)layerMinLengths[l] / (lowerEdgeLength + higherEdgeLength) * parameters.via_multiplier;
        CapacityT demand = parameters.UnitViaDemand;
        if (lowerEdgeLength > 0)
            cost += getWireCost(l, lowerLoc, demand);
        if (higherEdgeLength > 0)
            cost += getWireCost(l, loc, demand);
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
            if (point.layerIdx >= parameters.min_routing_layer) {
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
        // if (bestAccessDist.first == 0) {
        //     cout << "Warning: the pin is hard to access." << '\n';
        // } 
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

void GridGraph::commit(const int layerIndex, const utils::PointT<int> lower, const CapacityT demand) {
    graphEdges[layerIndex][lower.x][lower.y].demand += demand;
    assert(graphEdges[layerIndex][lower.x][lower.y].demand > -1);
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
        // CapacityT demand = (CapacityT)layerMinLengths[l] / (lowerEdgeLength + higherEdgeLength) * parameters.via_multiplier;
        CapacityT demand = parameters.UnitViaDemand;
        assert(demand > -1);
        if (lowerEdgeLength > 0)
            commit(l, lowerLoc, (reverse ? -demand : demand));
        if (higherEdgeLength > 0)
            commit(l, loc, (reverse ? -demand : demand));
    }
    // for (int l = layerIndex; l <= layerIndex + 1; l++) {
    //     commit(l, loc, (reverse ? -0.5 : 0.5));
    // }
    if (reverse)
        totalNumVias -= 1;
    else
        totalNumVias += 1;

    assert(totalNumVias >= 0);
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

bool GridGraph::checkOverflow_stage(const int layerIndex, const int x, const int y, int stage) const {
    if(stage == 3){
        return getEdge(layerIndex, x, y).getResource() < 0;
    }
    else if(stage == 2){
        return getEdge(layerIndex, x, y).getResource() < -1;
    }
    // stage == 1
    return getEdge(layerIndex, x, y).getResource() < -2;
}


int GridGraph::checkOverflow(const int layerIndex, const utils::PointT<int> u, const utils::PointT<int> v, int stage) const {
    int num = 0;
    unsigned direction = layerDirections[layerIndex];
    if (direction == 0) {
        assert(u.y == v.y);
        int l = min(u.x, v.x), h = max(u.x, v.x);
        for (int x = l; x < h; x++) {
            if (checkOverflow_stage(layerIndex, x, u.y, stage))
                num++;
        }
    } else {
        assert(u.x == v.x);
        int l = min(u.y, v.y), h = max(u.y, v.y);
        for (int y = l; y < h; y++) {
            if (checkOverflow_stage(layerIndex, u.x, y, stage))
                num++;
        }
    }
    return num;
}

int GridGraph::checkOverflow(const std::shared_ptr<GRTreeNode>& tree, int stage) const {
    if (!tree)
        return 0;
    int num = 0;
    GRTreeNode::preorder(tree, [&](std::shared_ptr<GRTreeNode> node) {
        for (auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                num += checkOverflow(node->layerIdx, (utils::PointT<int>)*node, (utils::PointT<int>)*child, stage);
            }
            else {
                assert(node->x == child->x && node->y == child->y);
                int maxLayerIndex = max(node->layerIdx, child->layerIdx);
                for (int layerIdx = min(node->layerIdx, child->layerIdx) + 1; layerIdx < maxLayerIndex - 1; layerIdx++) {
                    num += checkOverflow_stage(layerIdx, node->x, node->y, stage); 
                }
            }
        }
    });
    return num;
}

std::string GridGraph::getPythonString(const std::shared_ptr<GRTreeNode>& routingTree) const {
    vector<std::tuple<utils::PointT<int>, utils::PointT<int>, bool>> edges;
    GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node) {
        for (auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                unsigned direction = getLayerDirection(node->layerIdx);
                int r = (*node)[1 - direction];
                const int l = min((*node)[direction], (*child)[direction]);
                const int h = max((*node)[direction], (*child)[direction]);
                if (l == h)
                    continue;
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
                if (lpoint != hpoint)
                    edges.emplace_back(lpoint, hpoint, congested);
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

void GridGraph::extractBlockageView(GridGraphView<bool>& view) const {
    view.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize, true)));
    for (int layerIndex = parameters.min_routing_layer; layerIndex < nLayers; layerIndex++) {
        unsigned direction = getLayerDirection(layerIndex);
        for (int x = 0; x < xSize; x++) {
            for (int y = 0; y < ySize; y++) {
                if (getEdge(layerIndex, x, y).capacity >= 1.0) {
                    view[direction][x][y] = false;
                }
            }
        }
    }
}

void GridGraph::extractCongestionView(GridGraphView<bool>& view) const {
    view.assign(2, vector<vector<bool>>(xSize, vector<bool>(ySize, false)));
    for (int layerIndex = parameters.min_routing_layer; layerIndex < nLayers; layerIndex++) {
        unsigned direction = getLayerDirection(layerIndex);
        for (int x = 0; x < xSize; x++) {
            for (int y = 0; y < ySize; y++) {
                if (checkOverflow(layerIndex, x, y)) {
                    view[direction][x][y] = true;
                }
            }
        }
    }
}

void GridGraph::extractWireCostView(GridGraphView<CostT>& view) const {
    view.assign(2, vector<vector<CostT>>(xSize, vector<CostT>(ySize, std::numeric_limits<CostT>::max())));
    for (unsigned direction = 0; direction < 2; direction++) {
        vector<int> layerIndices;
        for (int layerIndex = parameters.min_routing_layer; layerIndex < getNumLayers(); layerIndex++) {
            if (getLayerDirection(layerIndex) == direction) {
                layerIndices.emplace_back(layerIndex);
            }
        }
        for (int x = 0; x < xSize; x++) {
            for (int y = 0; y < ySize; y++) {
                int edgeIndex = direction == 0 ? x : y;
                if (edgeIndex >= getSize(direction) - 1)
                    continue;
                CapacityT capacity = 0;
                CapacityT demand = 0;
                for (int layerIndex : layerIndices) {
                    const auto& edge = getEdge(layerIndex, x, y);
                    capacity += edge.capacity;
                    demand += edge.demand;
                    assert(capacity >= 0);
                    assert(demand > -1);
                }
                DBU length = getEdgeLength(direction, edgeIndex);
                view[direction][x][y] = length * UnitLengthWireCost;
            }
        }
    }
}

void GridGraph::updateWireCostView(GridGraphView<CostT>& view, std::shared_ptr<GRTreeNode> routingTree) const {
    vector<vector<int>> sameDirectionLayers(2);
    for (int layerIndex = parameters.min_routing_layer; layerIndex < getNumLayers(); layerIndex++) {
        unsigned direction = getLayerDirection(layerIndex);
        sameDirectionLayers[direction].emplace_back(layerIndex);
    }
    auto update = [&](unsigned direction, int x, int y) {
        int edgeIndex = direction == 0 ? x : y;
        if (edgeIndex >= getSize(direction) - 1)
            return;
        CapacityT capacity = 0;
        CapacityT demand = 0;
        for (int layerIndex : sameDirectionLayers[direction]) {
            if (getLayerDirection(layerIndex) != direction)
                continue;
            const auto& edge = getEdge(layerIndex, x, y);
            capacity += edge.capacity;
            demand += edge.demand;
            assert(capacity >= 0);
            assert(demand > -1);
        }
        DBU length = getEdgeLength(direction, edgeIndex);

        view[direction][x][y] = length * UnitLengthWireCost;
    };
    GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node) {
        for (const auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                unsigned direction = getLayerDirection(node->layerIdx);
                if (direction == 0) {
                    assert(node->y == child->y);
                    int l = min(node->x, child->x), h = max(node->x, child->x);
                    for (int x = l; x < h; x++) {
                        update(direction, x, node->y);
                    }
                } else {
                    assert(node->x == child->x);
                    int l = min(node->y, child->y), h = max(node->y, child->y);
                    for (int y = l; y < h; y++) {
                        update(direction, node->x, y);
                    }
                }
            } else {
                int maxLayerIndex = max(node->layerIdx, child->layerIdx);
                for (int layerIdx = min(node->layerIdx, child->layerIdx); layerIdx < maxLayerIndex; layerIdx++) {
                    unsigned direction = getLayerDirection(layerIdx);
                    update(direction, node->x, node->y);
                    if ((*node)[direction] > 0)
                        update(direction, node->x - 1 + direction, node->y - direction);
                }
            }
        }
    });
}

// void GridGraph::write(const std::string heatmap_file) const {
//     cout << "writing heatmap to file..." << '\n';
//     std::stringstream ss;

//     ss << nLayers << " " << xSize << " " << ySize << " " << '\n';
//     for (int layerIndex = 0; layerIndex < nLayers; layerIndex++) {
//         // ss << layerNames[layerIndex] << '\n';
//         ss << to_string(layerIndex) << '\n';
//         for (int y = 0; y < ySize; y++) {
//             for (int x = 0; x < xSize; x++) {
//                 ss << (graphEdges[layerIndex][x][y].capacity - graphEdges[layerIndex][x][y].demand)
//                    << (x == xSize - 1 ? "" : " ");
//             }
//             ss << '\n';
//         }
//     }
//     std::ofstream fout(heatmap_file);
//     fout << ss.str();
//     fout.close();
// }
