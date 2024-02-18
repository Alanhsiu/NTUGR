#include "GlobalRouter.h"
#include "PatternRoute.h"
#include "MazeRoute.h"
#include <chrono>

GlobalRouter::GlobalRouter(const Design& design, const Parameters& params): 
    gridGraph(design, params), parameters(params) {
    // Instantiate the global routing netlist
    const vector<Net>& baseNets = design.netlist.nets;
    nets.reserve(baseNets.size());
    for (const Net& baseNet : baseNets) {
        nets.emplace_back(baseNet, design, gridGraph); // GRNet(baseNet, design, gridGraph)
    }
}

void GlobalRouter::route() {
    int n1 = 0, n2 = 0, n3 = 0; // for statistics
    double t1 = 0, t2 = 0, t3 = 0; // for timing
    auto t = std::chrono::high_resolution_clock::now();

    vector<int> netIndices;
    netIndices.reserve(nets.size());
    for (const auto& net : nets) netIndices.push_back(net.getIndex());

    // Stage 1: Pattern routing
    n1 = netIndices.size();
    PatternRoute::readFluteLUT();
    cout << "stage 1: pattern routing" << endl;
    sortNetIndices(netIndices);
    for (const int netIndex : netIndices) {
        PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
        patternRoute.constructSteinerTree();
        patternRoute.constructRoutingDAG();
        patternRoute.run();
        gridGraph.commitTree(nets[netIndex].getRoutingTree());
    }

    netIndices.clear();
    for (const auto& net : nets) {
        if (gridGraph.checkOverflow(net.getRoutingTree()) > 0) {
            netIndices.push_back(net.getIndex());
        }
    }
    // print net indices with overflow
    // for (int netIndex : netIndices) {
    //     cout << "net " << netIndex << " has overflow." << endl;
    // }
    cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << endl;
    t1 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    t = std::chrono::high_resolution_clock::now();

    cout << "stage 1 time elapsed: " << t1 << endl;

    // Stage 2: Pattern routing with possible detours
    n2 = netIndices.size();
    if (netIndices.size() > 0) {
        cout << "stage 2: pattern routing with possible detours" << endl;
        GridGraphView<bool> congestionView; // (2d) direction -> x -> y -> has overflow?
        gridGraph.extractCongestionView(congestionView);

        sortNetIndices(netIndices);
        for (const int netIndex : netIndices) {
            GRNet& net = nets[netIndex];
            gridGraph.commitTree(net.getRoutingTree(), true);
            PatternRoute patternRoute(net, gridGraph, parameters);
            patternRoute.constructSteinerTree();
            patternRoute.constructRoutingDAG();
            patternRoute.constructDetours(congestionView); // KEY DIFFERENCE compared to stage 1
            patternRoute.run();
            gridGraph.commitTree(net.getRoutingTree());
        }
        
        netIndices.clear();
        for (const auto& net : nets) {
            if (gridGraph.checkOverflow(net.getRoutingTree()) > 0) {
                netIndices.push_back(net.getIndex());
            }
        }
        cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << endl;
    }
    t2 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    t = std::chrono::high_resolution_clock::now();

    cout << "stage 2 time elapsed: " << t2 << endl;

    // Stage 3: maze routing on sparsified routing graph
    n3 = netIndices.size();
    if (netIndices.size() > 0) {
        cout << "stage 3: maze routing on sparsified routing graph" << endl;
        for (const int netIndex : netIndices) {
            GRNet& net = nets[netIndex];
            gridGraph.commitTree(net.getRoutingTree(), true);
        }
        GridGraphView<CostT> wireCostView;
        gridGraph.extractWireCostView(wireCostView);
        sortNetIndices(netIndices);
        SparseGrid grid(10, 10, 0, 0);
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
            if (gridGraph.checkOverflow(net.getRoutingTree()) > 0) {
                netIndices.push_back(net.getIndex());
            }
        }
        cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << endl;
    }
    
    t3 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    t = std::chrono::high_resolution_clock::now();

    cout << "stage 1: " << n1 << " nets, " << std::setprecision(3) << std::fixed << t1 << " seconds" << endl;
    cout << "stage 2: " << n2 << " nets, " << std::setprecision(3) << std::fixed << t2 << " seconds" << endl;
    cout << "stage 3: " << n3 << " nets, " << std::setprecision(3) << std::fixed << t3 << " seconds" << endl;
    
    printStatistics();
    if (parameters.write_heatmap) gridGraph.write();
}

void GlobalRouter::sortNetIndices(vector<int>& netIndices) const { // sort by half perimeter: 短的先繞
    vector<int> halfParameters(nets.size());
    for (int netIndex : netIndices) {
        auto& net = nets[netIndex];
        halfParameters[netIndex] = net.getBoundingBox().hp();
    }
    sort(netIndices.begin(), netIndices.end(), [&](int lhs, int rhs) {
        return halfParameters[lhs] < halfParameters[rhs];
    });
}

void GlobalRouter::getGuides(const GRNet& net, vector<std::pair<int, utils::BoxT<int>>>& guides) {
    auto& routingTree = net.getRoutingTree();
    if (!routingTree) return;
    // 0. Basic guides
    GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node) {
        for (const auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                guides.emplace_back(
                    node->layerIdx, utils::BoxT<int>(
                        min(node->x, child->x), min(node->y, child->y),
                        max(node->x, child->x), max(node->y, child->y)
                    )
                );
            } else {
                int maxLayerIndex = max(node->layerIdx, child->layerIdx);
                for (int layerIdx = min(node->layerIdx, child->layerIdx); layerIdx <= maxLayerIndex; layerIdx++) {
                    guides.emplace_back(layerIdx, utils::BoxT<int>(node->x, node->y));
                }
            }
        }
    });
}

void GlobalRouter::printStatistics() const {
    cout << "routing statistics" << endl;

    // wire length and via count
    uint64_t wireLength = 0;
    int viaCount = 0;
    vector<vector<vector<int>>> wireUsage;
    wireUsage.assign(
        gridGraph.getNumLayers(), vector<vector<int>>(gridGraph.getSize(0), vector<int>(gridGraph.getSize(1), 0))
    );
    for (const auto& net : nets) {
        GRTreeNode::preorder(net.getRoutingTree(), [&] (std::shared_ptr<GRTreeNode> node) {
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
    
    cout << "wire length (metric):  " << wireLength << endl;
    cout << "total via count:       " << viaCount << endl;
    cout << "total wire overflow:   " << (int)overflow << endl;

    cout << "min resource: " << minResource << endl;
    cout << "bottleneck:   " << bottleneck << endl;

}


void GlobalRouter::write(std::string guide_file) {
    cout << "generating route guides..." << endl;
    if (guide_file == "") guide_file = parameters.out_file;
    
    std::stringstream ss;
    for (const GRNet& net : nets) {
        vector<std::pair<int, utils::BoxT<int>>> guides;
        getGuides(net, guides);
        
        ss << net.getName() << endl;
        ss << "(" << endl;
        
        // --- modified by Alan ---
        std::map<std::pair<int, int>, std::pair<int, int>> verticalGuides;
        for (const auto& guide : guides) {
            // Store vertical guides to add vias later
            if (guide.second.x.low == guide.second.x.high && guide.second.y.low == guide.second.y.high) {
                auto key = std::make_pair(guide.second.x.low, guide.second.y.low);
                auto& value = verticalGuides[key];
                value.first = std::min(value.first, guide.first);
                value.second = std::max(value.second, guide.first);
            }
            else{
                ss  << guide.second.x.low << " "
                    << guide.second.y.low << " "
                    << guide.first << " "
                    << guide.second.x.high << " "
                    << guide.second.y.high << " "
                    << guide.first << "\n";
            }
        }

        // Add vias for vertical guides
        for (const auto& pair : verticalGuides) {
            for (int z = pair.second.first; z < pair.second.second; ++z) {
                ss << pair.first.first << " "
                   << pair.first.second << " "
                   << z << " "
                   << pair.first.first << " "
                   << pair.first.second << " "
                   << z + 1 << endl;
            }
        }
        ss << ")" << endl;

        // --- end of modification ---

        // for (const auto& guide : guides) {
        //     ss << guide.second.x.low << " "
        //         << guide.second.y.low << " "
        //         << guide.first << " "
        //         << guide.second.x.high+1 << " "
        //         << guide.second.y.high+1 << " "
        //         << guide.first << endl;
        // }
        // ss << ")" << endl;
        // for (const auto& guide : guides) {
        //     ss << gridGraph.getGridline(0, guide.second.x.low) << " "
        //          << gridGraph.getGridline(1, guide.second.y.low) << " "
        //          << gridGraph.getGridline(0, guide.second.x.high + 1) << " "
        //          << gridGraph.getGridline(1, guide.second.y.high + 1) << " "
        //         //  << gridGraph.getLayerName(guide.first) << endl;
        //             << guide.first << endl;
        // }
        // ss << ")" << endl;
    }

    cout << endl;
    cout << "writing output..." << endl;
    std::ofstream fout(guide_file);
    fout << ss.str();
    fout.close();
    cout << "finished writing output..." << endl;
}
