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
    
    inline unsigned getNumLayers() const { return nLayers; }
    inline unsigned getSize(unsigned dimension) const { return (dimension ? ySize : xSize); }
    inline unsigned getLayerDirection(int layerIndex) const { return layerDirections[layerIndex]; }

    inline uint64_t hashCell(const GRPoint& point) const { // hash a point to a unique number
        return ((uint64_t)point.layerIdx * xSize + point.x) * ySize + point.y;
    };
    inline uint64_t hashCell(const int x, const int y) const { return (uint64_t)x * ySize + y; }
    inline DBU getGridline(const unsigned dimension, const int index) const { return gridlines[dimension][index]; }
    utils::BoxT<DBU> getCellBox(utils::PointT<int> point) const;
    utils::BoxT<int> rangeSearchCells(const utils::BoxT<DBU>& box) const;
    inline GraphEdge getEdge(const int layerIndex, const int x, const int y) const {return graphEdges[layerIndex][x][y]; }

    // Costs
    DBU getEdgeLength(unsigned direction, unsigned edgeIndex) const;
    double getWireCost(const int layerIndex, const utils::PointT<int> u, const utils::PointT<int> v) const;
    double getViaCost(const int layerIndex, const utils::PointT<int> loc) const;
    inline double getUnitViaCost() const { return UnitViaCost; }
    
    // Misc
    void selectAccessPoints(GRNet& net, robin_hood::unordered_map<uint64_t, std::pair<utils::PointT<int>, utils::IntervalT<int>>>& selectedAccessPoints) const;
    
    // Methods for updating demands 
    void commitTree(const std::shared_ptr<GRTreeNode>& tree, const bool reverse = false);
    
    // Checks
    inline bool checkOverflow(const int layerIndex, const int x, const int y) const { return getEdge(layerIndex, x, y).getResource() < 0.0; }
    int checkOverflow(const int layerIndex, const utils::PointT<int> u, const utils::PointT<int> v) const; // Check wire overflow
    int checkOverflow(const std::shared_ptr<GRTreeNode>& tree) const; // Check routing tree overflow (Only wires are checked)
    std::string getPythonString(const std::shared_ptr<GRTreeNode>& routingTree) const;
   

// private:
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
    
    inline double getUnitLengthWireCost() const { return UnitLengthWireCost; }
    double getWireCost(const int layerIndex, const utils::PointT<int> lower, const double demand = 1.0) const;

    inline double logistic(const double& input, const double slope) const;

    // Methods for updating demands 
    void commit(const int layerIndex, const utils::PointT<int> lower, const double demand);
    void commitWire(const int layerIndex, const utils::PointT<int> lower, const bool reverse = false);
    void commitVia(const int layerIndex, const utils::PointT<int> loc, const bool reverse = false);

};

