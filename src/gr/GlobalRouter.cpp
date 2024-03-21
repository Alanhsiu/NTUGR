
#include "GlobalRouter.h"
#include "PatternRoute.h"
#include "MazeRoute.h"
#include <chrono>

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
    bool stage2 = true;
    bool stage3 = false;

    vector<int> netIndices;
    netIndices.reserve(nets.size());
    for (const auto& net : nets)
        netIndices.emplace_back(net.getIndex());
    int overflowThreshold = -2;
    cout << "nets size: " << nets.size() << ", overflowThreshold: " << overflowThreshold << std::endl;

    PatternRoute::readFluteLUT();

    // Stage 1: Pattern routing
    auto t1 = std::chrono::high_resolution_clock::now();
    auto temp = std::chrono::high_resolution_clock::now();
    n1 = netIndices.size();
    cout << "Stage 1: Pattern routing" << std::endl;

    int threadNum = 4;
    vector<vector<int>> nonoverlapNetIndices;
    nonoverlapNetIndices.resize(threadNum + 1);  // 8 for 8 threads, 1 for the rest

    sortNetIndices(netIndices);

    /* Separate 1 */
    separateNetIndices(netIndices, nonoverlapNetIndices);
    for (int i = 0; i < nonoverlapNetIndices.size(); ++i)
        cout << "thread " << i << " size: " << nonoverlapNetIndices[i].size() << std::endl;

    /* Non-parallel 1 */
    int k = 0;
    for (int i = 0; i < k; ++i) {
        int netIndex = nonoverlapNetIndices[threadNum][i];
        PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
        patternRoute.constructSteinerTree();
        patternRoute.constructRoutingDAG();
        patternRoute.run();
        std::shared_ptr<GRTreeNode> tree = nets[netIndex].getRoutingTree();
        gridGraph.commitTree(tree);
    }
    cout << "non-parallel for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - temp).count() << std::endl;
    temp = std::chrono::high_resolution_clock::now();

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
    cout << "parallel 1 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - temp).count() << std::endl;
    temp = std::chrono::high_resolution_clock::now();

    /* Non-parallel 2 */
    for (int i = k; i < nonoverlapNetIndices[threadNum].size(); ++i) {
        int netIndex = nonoverlapNetIndices[threadNum][i];
        PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
        patternRoute.constructSteinerTree();
        patternRoute.constructRoutingDAG();
        patternRoute.run();
        std::shared_ptr<GRTreeNode> tree = nets[netIndex].getRoutingTree();
        gridGraph.commitTree(tree);
    }
    cout << "non-parallel 2 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - temp).count() << std::endl;
    temp = std::chrono::high_resolution_clock::now();

    cout << "stage 1 time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t1).count() << std::endl;

    netIndices.clear();
    for (const auto& net : nets) {
        if (gridGraph.checkOverflow(net.getRoutingTree(), 1) > 0) { // change to stage
            netIndices.push_back(net.getIndex());
        }
    }
    cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << std::endl;

    // Stage 2: Pattern routing with possible detours
    auto t2 = std::chrono::high_resolution_clock::now();
    n2 = netIndices.size();
    if (stage2 && netIndices.size() > 0) {
        cout << "Stage 2: Pattern routing with possible detours" << std::endl;
        GridGraphView<bool> congestionView;  // (2d) direction -> x -> y -> has overflow?
        gridGraph.extractCongestionView(congestionView);

        /* Separate 2 */
        vector<vector<int>> nonoverlapNetIndices;
        nonoverlapNetIndices.resize(threadNum + 1);
        separateNetIndices(netIndices, nonoverlapNetIndices);

#pragma omp parallel for
        for (int i = 0; i < nonoverlapNetIndices.size(); ++i) {
            sortNetIndices(nonoverlapNetIndices[i]);
        }
        for (int i = 0; i < nonoverlapNetIndices.size(); ++i)
            cout << "thread " << i << " size: " << nonoverlapNetIndices[i].size() << std::endl;

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
        cout << "parallel 2 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - temp).count() << std::endl;
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
        cout << "non-parallel 2 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - temp).count() << std::endl;
        temp = std::chrono::high_resolution_clock::now();

        netIndices.clear();
        for (const auto& net : nets) {
            if (gridGraph.checkOverflow(net.getRoutingTree(), 2) > 0) { // change to stage
                netIndices.push_back(net.getIndex());
            }
        }
        cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << std::endl;

        cout << "stage 2 time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t2).count() << std::endl;
        temp = std::chrono::high_resolution_clock::now();
    }

    // Stage 3: maze routing on sparsified routing graph
    auto t3 = std::chrono::high_resolution_clock::now();
    n3 = netIndices.size();
    if (stage3 && netIndices.size() > 0) {
        cout << "stage 3: maze routing on sparsified routing graph" << '\n';
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
        cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << '\n';
        cout << "stage 3 time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t3).count() << std::endl;
    }

    printStatistics();
    if (parameters.write_heatmap) gridGraph.write();
}

void GlobalRouter::separateNetIndices(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices) const {  // separate nets such that nets are routed in parallel
    // separate nets into 4 groups, and the rest
    int netSize = netIndices.size();
    for (int netIndex : netIndices) {
        const GRNet& net = nets[netIndex];
        const auto& boundingBox = net.getBoundingBox();

        int xlow = boundingBox.x.low, xhigh = boundingBox.x.high, ylow = boundingBox.y.low, yhigh = boundingBox.y.high;
        int xSize = gridGraph.getSize(0), ySize = gridGraph.getSize(1);

        if (xhigh < xSize / 2) {
            if (yhigh < ySize / 2) {
                nonoverlapNetIndices[0].push_back(netIndex);
                continue;
            } else if (ylow > ySize / 2) {
                nonoverlapNetIndices[1].push_back(netIndex);
                continue;
            }
        } else if (xlow > xSize / 2) {
            if (yhigh < ySize / 2) {
                nonoverlapNetIndices[2].push_back(netIndex);
                continue;
            } else if (ylow > ySize / 2) {
                nonoverlapNetIndices[3].push_back(netIndex);
                continue;
            }
        }
        nonoverlapNetIndices[4].push_back(netIndex);
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
    cout << "routing statistics" << std::endl;

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

    cout << "wire length (metric):  " << wireLength << std::endl;
    cout << "total via count:       " << viaCount << std::endl;
    cout << "total wire overflow:   " << (int)overflow << std::endl;

    cout << "min resource: " << minResource << std::endl;
    cout << "bottleneck:   " << bottleneck << std::endl;
}

void GlobalRouter::write(std::string guide_file) {
    cout << "generating route guides..." << std::endl;
    if (guide_file == "")
        guide_file = parameters.out_file;

    auto start = std::chrono::high_resolution_clock::now();
    int netSize = nets.size();
#pragma omp parallel for
    for (int i = 0; i < netSize; i++) {
        nets[i].getGuides();
    }
    cout << "Elapsed time for getGuides: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << " s" << endl;

    FILE* out;
    out = fopen(guide_file.c_str(), "w");
    for (const GRNet& net : nets) {
        fprintf(out, "%s", net.guide_string.c_str());
    }
    fclose(out);

    cout << std::endl;
    cout << "finished writing output..." << std::endl;
}