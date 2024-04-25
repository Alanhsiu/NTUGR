
#include "GlobalRouter.h"
#include <chrono>
#include "MazeRoute.h"
#include "PatternRoute.h"

GlobalRouter::GlobalRouter(const Design& design, const Parameters& params)
    : gridGraph(design, params), parameters(params) {
    // Instantiate the global routing netlist
    const size_t numNets = design.netlist.nets.size();
    nets.reserve(numNets);
    for (const Net& baseNet : design.netlist.nets) {
        nets.emplace_back(baseNet, design, gridGraph);
    }
}

void GlobalRouter::route() {
    int n1 = 0, n2 = 0, n3 = 0;  // for statistics
    int threadNum = 8;
    bool stage2 = true;
    bool stage3 = false;

    vector<int> netIndices;
    netIndices.reserve(nets.size());
    for (const auto& net : nets)
        netIndices.emplace_back(net.getIndex());

    PatternRoute::readFluteLUT();

    // Stage 1: Pattern routing
    auto t1 = std::chrono::high_resolution_clock::now();
    auto temp = std::chrono::high_resolution_clock::now();
    n1 = netIndices.size();
    std::cout << "Stage 1: Pattern routing" << std::endl;

    GridGraphView<bool> congestionView;  // (2d) direction -> x -> y -> has overflow?
    gridGraph.extractCongestionView(congestionView);

    sortNetIndices(netIndices);

    /* Separate 1 */
    vector<vector<int>> nonoverlapNetIndices;
    nonoverlapNetIndices.resize(threadNum + 1);  // 8 for 8 threads, 1 for the rest
    separateNetIndices(netIndices, nonoverlapNetIndices);

    for (int i = 0; i < nonoverlapNetIndices.size(); ++i)
        std::cout << "thread " << i << " size: " << nonoverlapNetIndices[i].size() << std::endl;

    /* Parallel 1 */
    omp_lock_t lock;
    omp_init_lock(&lock);

#pragma omp parallel for
    for (int i = 0; i < threadNum; ++i) {
        for (int j = 0; j < nonoverlapNetIndices[i].size(); ++j) {
            int netIndex = nonoverlapNetIndices[i][j];
            PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
            omp_set_lock(&lock);
            patternRoute.constructSteinerTree();
            omp_unset_lock(&lock);
            patternRoute.constructRoutingDAG();
            patternRoute.run();
            std::shared_ptr<GRTreeNode> tree = nets[netIndex].getRoutingTree();
            gridGraph.commitTree(tree);
        }
    }
    std::cout << "parallel 1 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - temp).count() << std::endl;
    temp = std::chrono::high_resolution_clock::now();

    /* Non-parallel 1 */
    for (int i = 0; i < nonoverlapNetIndices[threadNum].size(); ++i) {
        int netIndex = nonoverlapNetIndices[threadNum][i];
        PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
        patternRoute.constructSteinerTree();
        patternRoute.constructRoutingDAG();
        patternRoute.run();
        std::shared_ptr<GRTreeNode> tree = nets[netIndex].getRoutingTree();
        gridGraph.commitTree(tree);
    }
    std::cout << "non-parallel 1 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - temp).count() << std::endl;
    temp = std::chrono::high_resolution_clock::now();

    std::cout << "stage 1 time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t1).count() << std::endl;

    netIndices.clear();
    for (const auto& net : nets) {
        if (gridGraph.checkOverflow(net.getRoutingTree(), 0) > 0) {  // change to stage
            netIndices.push_back(net.getIndex());
        }
    }
    std::cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << std::endl;

    // Stage 2: Pattern routing with possible detours
    auto t2 = std::chrono::high_resolution_clock::now();
    n2 = netIndices.size();
    if (stage2 && netIndices.size() > 0) {
        std::cout << "Stage 2: Pattern routing with possible detours" << std::endl;
        GridGraphView<bool> congestionView;  // (2d) direction -> x -> y -> has overflow?
        gridGraph.extractCongestionView(congestionView);

        sortNetIndices(netIndices);

        /* Separate 2 */
        vector<vector<int>> nonoverlapNetIndices;
        nonoverlapNetIndices.resize(threadNum + 1);
        separateNetIndices(netIndices, nonoverlapNetIndices);

        // #pragma omp parallel for
        //         for (int i = 0; i < nonoverlapNetIndices.size(); ++i)
        //             sortNetIndices(nonoverlapNetIndices[i]);
        for (int i = 0; i < nonoverlapNetIndices.size(); ++i)
            std::cout << "thread " << i << " size: " << nonoverlapNetIndices[i].size() << std::endl;

        /* Parallel 2 */
        omp_lock_t lock2;
        omp_init_lock(&lock2);

#pragma omp parallel for
        for (int i = 0; i < threadNum; ++i) {
            for (int j = 0; j < nonoverlapNetIndices[i].size(); ++j) {
                int netIndex = nonoverlapNetIndices[i][j];
                GRNet& net = nets[netIndex];
                gridGraph.commitTree(net.getRoutingTree(), true);
                PatternRoute patternRoute(net, gridGraph, parameters);
                omp_set_lock(&lock2);
                patternRoute.constructSteinerTree();
                omp_unset_lock(&lock2);
                patternRoute.constructRoutingDAG();
                patternRoute.constructDetours(congestionView);  // KEY DIFFERENCE compared to stage 1
                patternRoute.run();
                gridGraph.commitTree(net.getRoutingTree());
            }
        }
        std::cout << "parallel 2 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - temp).count() << std::endl;
        temp = std::chrono::high_resolution_clock::now();

        /* Non-parallel 2 */
        for (int i = 0; i < nonoverlapNetIndices[threadNum].size(); ++i) {
            int netIndex = nonoverlapNetIndices[threadNum][i];
            GRNet& net = nets[netIndex];
            gridGraph.commitTree(net.getRoutingTree(), true);
            PatternRoute patternRoute(net, gridGraph, parameters);
            patternRoute.constructSteinerTree();
            patternRoute.constructRoutingDAG();
            patternRoute.constructDetours(congestionView);  // KEY DIFFERENCE compared to stage 1
            patternRoute.run();
            gridGraph.commitTree(net.getRoutingTree());
        }
        std::cout << "non-parallel 2 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - temp).count() << std::endl;
        temp = std::chrono::high_resolution_clock::now();

        netIndices.clear();
        for (const auto& net : nets) {
            if (gridGraph.checkOverflow(net.getRoutingTree(), 0) > 0) {  // change to stage
                netIndices.push_back(net.getIndex());
            }
        }
        std::cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << std::endl;

        std::cout << "stage 2 time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t2).count() << std::endl;
        temp = std::chrono::high_resolution_clock::now();
    }
    // Stage 3: maze routing on sparsified routing graph
    auto t3 = std::chrono::high_resolution_clock::now();
    n3 = netIndices.size();
    if (stage3 && netIndices.size() > 0) {
        std::cout << "stage 3: maze routing on sparsified routing graph" << '\n';
        for (const int netIndex : netIndices) {
            GRNet& net = nets[netIndex];
            gridGraph.commitTree(net.getRoutingTree(), true);
        }
        GridGraphView<CostT> wireCostView;
        gridGraph.extractWireCostView(wireCostView);
        sortNetIndices(netIndices);
        SparseGrid grid(1, 1, 0, 0);
        // SparseGrid grid(10, 10, 0, 0);
        for (const int netIndex : netIndices) {
            GRNet& net = nets[netIndex];
            // gridGraph.commitTree(net.getRoutingTree(), true);
            // gridGraph.updateWireCostView(wireCostView, net.getRoutingTree());
            MazeRoute mazeRoute(net, gridGraph, parameters);
            mazeRoute.constructSparsifiedGraph(wireCostView, grid);
            mazeRoute.run();
            std::shared_ptr<SteinerTreeNode> tree = mazeRoute.getSteinerTree();
            assert(tree != nullptr);

            PatternRoute patternRoute(net, gridGraph, parameters);
            patternRoute.setSteinerTree(tree);
            patternRoute.constructRoutingDAG();
            patternRoute.run();

            gridGraph.commitTree(net.getRoutingTree());
            gridGraph.updateWireCostView(wireCostView, net.getRoutingTree());
            grid.step();
        }
        netIndices.clear();
        for (const auto& net : nets) {
            if (gridGraph.checkOverflow(net.getRoutingTree(), 2) > 0) {
                netIndices.push_back(net.getIndex());
            }
        }
        std::cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << '\n';
        std::cout << "stage 3 time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t3).count() << std::endl;
    }

    printStatistics();
    if (parameters.write_heatmap)
        gridGraph.write();
}

void GlobalRouter::separateNetIndices(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices) const {  // separate nets such that nets are routed in parallel
    // separate nets into 8 groups (on x direction, don't consider y direction), and the rest
    int netSize = netIndices.size();

    // go through all nets from left to right
    vector<int> netCenterX(netSize);
    vector<int> divideX(7);

    for (int i = 0; i < netSize; i++) {
        const GRNet& net = nets[netIndices[i]];
        const auto& boundingBox = net.getBoundingBox();
        netCenterX[i] = boundingBox.cx();
    }
    sort(netCenterX.begin(), netCenterX.end());
    for (int i = 0; i < 7; i++) {
        divideX[i] = netCenterX[(i + 1) * netSize / 8];
    }

    for (int i = 0; i < netSize; i++) {
        const GRNet& net = nets[netIndices[i]];
        const auto& boundingBox = net.getBoundingBox();
        int xlow = boundingBox.x.low, xhigh = boundingBox.x.high, ylow = boundingBox.y.low, yhigh = boundingBox.y.high;
        // int xSize = gridGraph.getSize(0), ySize = gridGraph.getSize(1);

        if (xhigh < divideX[0]) {
            nonoverlapNetIndices[0].push_back(netIndices[i]);
            continue;
        } else if (xhigh < divideX[1] && xlow > divideX[0]) {
            nonoverlapNetIndices[1].push_back(netIndices[i]);
            continue;
        } else if (xhigh < divideX[2] && xlow > divideX[1]) {
            nonoverlapNetIndices[2].push_back(netIndices[i]);
            continue;
        } else if (xhigh < divideX[3] && xlow > divideX[2]) {
            nonoverlapNetIndices[3].push_back(netIndices[i]);
            continue;
        } else if (xhigh < divideX[4] && xlow > divideX[3]) {
            nonoverlapNetIndices[4].push_back(netIndices[i]);
            continue;
        } else if (xhigh < divideX[5] && xlow > divideX[4]) {
            nonoverlapNetIndices[5].push_back(netIndices[i]);
            continue;
        } else if (xhigh < divideX[6] && xlow > divideX[5]) {
            nonoverlapNetIndices[6].push_back(netIndices[i]);
            continue;
        } else if (xlow > divideX[6]) {
            nonoverlapNetIndices[7].push_back(netIndices[i]);
            continue;
        }
        nonoverlapNetIndices[8].push_back(netIndices[i]);
    }
}

void GlobalRouter::sortNetIndices(vector<int>& netIndices) const {  // sort by half perimeter: 短的先繞
    vector<int> halfParameters(nets.size());
    // vector<int> maxEdgeLength(nets.size()); // added by Alan
    for (int netIndex : netIndices) {
        auto& net = nets[netIndex];
        halfParameters[netIndex] = net.getBoundingBox().hp();
        // maxEdgeLength[netIndex] = max(net.getBoundingBox().x.range(), net.getBoundingBox().y.range()); // added by Alan
    }
    sort(netIndices.begin(), netIndices.end(), [&](int lhs, int rhs) {
        return halfParameters[lhs] < halfParameters[rhs];
        // return maxEdgeLength[lhs] < maxEdgeLength[rhs]; // added by Alan
    });
}

void GlobalRouter::printStatistics() const {
    std::cout << "routing statistics" << std::endl;

    // wire length and via count
    uint64_t wireLength = 0;
    int viaCount = 0;
    vector<vector<vector<int>>> wireUsage;
    wireUsage.assign(
        gridGraph.getNumLayers(), vector<vector<int>>(gridGraph.getSize(0), vector<int>(gridGraph.getSize(1), 0)));
    for (const auto& net : nets) {
        GRTreeNode::preorder(net.getRoutingTree(), [&](std::shared_ptr<GRTreeNode> node) {
            for (const auto& child : node->children) {
                if (node->layerIdx == child->layerIdx) {
                    unsigned direction = gridGraph.getLayerDirection(node->layerIdx);
                    int l = min((*node)[direction], (*child)[direction]);
                    int h = max((*node)[direction], (*child)[direction]);
                    int r = (*node)[1 - direction];
                    for (int c = l; c < h; c++) {
                        wireLength += gridGraph.getEdgeLength(direction, c);
                        int x = direction == 0 ? c : r;
                        int y = direction == 0 ? r : c;
                        wireUsage[node->layerIdx][x][y] += 1;
                    }
                } else {
                    viaCount += abs(node->layerIdx - child->layerIdx);
                }
            }
        });
    }

    // resource
    CapacityT overflow = 0;

    CapacityT minResource = std::numeric_limits<CapacityT>::max();
    GRPoint bottleneck(-1, -1, -1);
    for (int layerIndex = parameters.min_routing_layer; layerIndex < gridGraph.getNumLayers(); layerIndex++) {
        unsigned direction = gridGraph.getLayerDirection(layerIndex);
        for (int x = 0; x < gridGraph.getSize(0) - 1 + direction; x++) {
            for (int y = 0; y < gridGraph.getSize(1) - direction; y++) {
                CapacityT resource = gridGraph.getEdge(layerIndex, x, y).getResource();
                if (resource < minResource) {
                    minResource = resource;
                    bottleneck = {layerIndex, x, y};
                }
                CapacityT usage = wireUsage[layerIndex][x][y];
                CapacityT capacity = max(gridGraph.getEdge(layerIndex, x, y).capacity, 0.0);
                if (usage > 0.0 && usage > capacity) {
                    overflow += usage - capacity;
                }
            }
        }
    }

    std::cout << "wire length (metric):  " << wireLength << std::endl;
    std::cout << "total via count:       " << viaCount << std::endl;
    std::cout << "total wire overflow:   " << (int)overflow << std::endl;

    std::cout << "min resource: " << minResource << std::endl;
    std::cout << "bottleneck:   " << bottleneck << std::endl;
}

void GlobalRouter::write(std::string guide_file) {
    std::cout << "generating route guides..." << std::endl;
    if (guide_file == "")
        guide_file = parameters.out_file;

    auto start = std::chrono::high_resolution_clock::now();
    int netSize = nets.size();
#pragma omp parallel for
    for (int i = 0; i < netSize; i++) {
        nets[i].getGuides();
    }
    std::cout << "Elapsed time for getGuides: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << " s" << endl;

    FILE* out;
    out = fopen(guide_file.c_str(), "w");
    for (const GRNet& net : nets) {
        fprintf(out, "%s", net.guide_string.c_str());
    }
    fclose(out);

    std::cout << std::endl;
    std::cout << "finished writing output..." << std::endl;
}