#include "GRNet.h"

GRNet::GRNet(const Net& baseNet, const Design& design, const GridGraph& gridGraph): id(baseNet.id), name(baseNet.name) {
    pinAccessPoints.resize(baseNet.pin_ids.size());
    // TODO: construct pinAccessPoints
}
