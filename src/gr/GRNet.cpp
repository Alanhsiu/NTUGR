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
