#pragma once
#include "../global.h"
#include "../basic/design.h"
#include "GRTree.h"

class GRNet;

struct GraphEdge {
    GraphEdge(): capacity(0), demand(0) {}

    double capacity;
    double demand;
    double getResource() const { return capacity - demand; }
};

class GridGraph {
public:
    GridGraph(const Design& design, const Parameters& params);

private:
    const Parameters& parameters;

    unsigned nLayers;
    unsigned xSize;
    unsigned ySize;
    vector<vector<DBU>> gridlines;
    vector<vector<DBU>> gridCenters;
    vector<unsigned> layerDirections;
    vector<DBU> layerMinLengths;

    double UnitLengthWireCost;
    double UnitViaCost;
    vector<double> OFWeight; // overflow weights

    DBU totalLength = 0;
    int totalNumVias = 0;
    vector<vector<vector<GraphEdge>>> graphEdges; // gridEdges[l][x][y] stores the edge {(l, x, y), (l, x+1, y)} or {(l, x, y), (l, x, y+1)}, depending on the routing direction of the layer

    utils::IntervalT<int> rangeSearchGridlines(const unsigned dimension, const utils::IntervalT<DBU>& locInterval) const; // Find the gridlines within [locInterval.low, locInterval.high]
    utils::IntervalT<int> rangeSearchRows(const unsigned dimension, const utils::IntervalT<DBU>& locInterval) const; // Find the rows/columns overlapping with [locInterval.low, locInterval.high]

    // Methods for updating demands 
    void commit(const int layerIndex, const utils::PointT<int> lower, const double demand);
    void commitWire(const int layerIndex, const utils::PointT<int> lower, const bool reverse = false);
    void commitVia(const int layerIndex, const utils::PointT<int> loc, const bool reverse = false);

};

