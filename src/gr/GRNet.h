#pragma once
#include "../global.h"
#include "../basic/design.h"
#include "GridGraph.h"

class GRNet {
public:
    GRNet(const Net& baseNet, const Design& design, const GridGraph& gridGraph);
    
    int getIndex() const { return index; }
    std::string getName() const { return name; }
    int getNumPins() const { return pinAccessPoints.size(); }
    const vector<vector<GRPoint>>& getPinAccessPoints() const { return pinAccessPoints; }
    const utils::BoxT<int>& getBoundingBox() const { return boundingBox; }
    const std::shared_ptr<GRTreeNode>& getRoutingTree() const { return routingTree; }
    
    void setRoutingTree(std::shared_ptr<GRTreeNode> tree) { routingTree = tree; }
    void clearRoutingTree() { routingTree = nullptr; }
    
    int index;
    std::string name;
    vector<vector<GRPoint>> pinAccessPoints; // pinAccessPoints[pinIndex][accessPointIndex]
    utils::BoxT<int> boundingBox;
    std::shared_ptr<GRTreeNode> routingTree;
};