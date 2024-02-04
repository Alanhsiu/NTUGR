#pragma once
#include "../global.h"
#include "../basic/design.h"
#include "GridGraph.h"

class GRNet {
public:
    GRNet(const Net& baseNet, const Design& design, const GridGraph& gridGraph);
    
    int getNumPins() const { return pinAccessPoints.size(); }
    const vector<vector<GRPoint>>& getPinAccessPoints() const { return pinAccessPoints; }
    const utils::BoxT<int>& getBoundingBox() const { return boundingBox; }
    const std::shared_ptr<GRTreeNode>& getRoutingTree() const { return routingTree; }
    // void getGuides(vector<std::pair<int, utils::BoxT<int>>>& guides) const;
    
    void setRoutingTree(std::shared_ptr<GRTreeNode> tree) { routingTree = tree; }
    void clearRoutingTree() { routingTree = nullptr; }
    
    int id;
    std::string name;
    vector<vector<GRPoint>> pinAccessPoints;
    utils::BoxT<int> boundingBox;
    std::shared_ptr<GRTreeNode> routingTree;
};