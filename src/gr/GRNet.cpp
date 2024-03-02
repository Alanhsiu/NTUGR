#include "GRNet.h"

GRNet::GRNet(const Net& baseNet, const Design& design, const GridGraph& gridGraph): index(baseNet.id), name(baseNet.name) {
    pinAccessPoints.resize(baseNet.pin_ids.size());
    
    // construct pinAccessPoints
    for (int i = 0; i < baseNet.pin_ids.size(); i++) { // i is pinIndex
        const Pin& pin = design.netlist.pins[baseNet.pin_ids[i]];
        pinAccessPoints[i].resize(pin.point_ids.size());
        for (int j = 0; j < pin.point_ids.size(); j++) { // j is accessPointIndex
            const Point& point = design.netlist.points[pin.point_ids[j]];

            pinAccessPoints[i][j] = GRPoint(point.layer, point.x, point.y);
            // int x = gridGraph.searchXGridline(point.x);
            // int y = gridGraph.searchYGridline(point.y);
            // pinAccessPoints[i][j] = GRPoint(point.layer, x, y);
        }
    }

    // construct boundingBox
    boundingBox = utils::BoxT<int>(INT_MAX, INT_MAX, INT_MIN, INT_MIN);
    for (const auto& pinPoints: pinAccessPoints) {
        for (const auto& point: pinPoints) {
            boundingBox.lx() = std::min(boundingBox.lx(), point.x);
            boundingBox.hx() = std::max(boundingBox.hx(), point.x);
            boundingBox.ly() = std::min(boundingBox.ly(), point.y);
            boundingBox.hy() = std::max(boundingBox.hy(), point.y);
        }
    }
}

void GRNet::getGuides() {
    if (!routingTree)
        return;

    vector<vector<int>> guide;
    GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node) {
        for (const auto& child : node->children) {
            if(node->x == child->x && node->y == child->y && node->layerIdx == child->layerIdx){
                // cout << "Warning: node and child are the same" << endl;
                continue;
            }
            vector<int> vec;
            vec.reserve(6);
            vec.emplace_back(min(node->x, child->x));
            vec.emplace_back(min(node->y, child->y));
            vec.emplace_back(min(node->layerIdx, child->layerIdx));
            vec.emplace_back(max(node->x, child->x));
            vec.emplace_back(max(node->y, child->y));
            vec.emplace_back(max(node->layerIdx, child->layerIdx));
            guide.push_back(vec);
        }
    });
    // check if the guide has same elements
    unordered_set<string> guideSet;
    guide_string = name + "\n(\n";
    for (const auto& vec : guide) {
        string temp;    
        for (const auto& i : vec) {
            temp += to_string(i) + " ";
        }
        if (guideSet.find(temp) == guideSet.end()) {
            guideSet.insert(temp);
            guide_string += temp + "\n";
        }
    }
    guide_string += ")\n";
}
