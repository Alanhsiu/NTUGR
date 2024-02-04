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