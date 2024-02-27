
#include "GlobalRouter.h"
#include "PatternRoute.h"
// #include "MazeRoute.h"
#include <chrono>

GlobalRouter::GlobalRouter(const Design& design, const Parameters& params)
    : gridGraph(design, params), parameters(params) {
    // Instantiate the global routing netlist
    const vector<Net>& baseNets = design.netlist.nets;
    nets.reserve(baseNets.size());
    for (const Net& baseNet : baseNets) {
        nets.emplace_back(baseNet, design, gridGraph);  // GRNet(baseNet, design, gridGraph)
    }
}

void GlobalRouter::route() {
    int n1 = 0, n2 = 0, n3 = 0;  // for statistics
    auto t = std::chrono::high_resolution_clock::now();

    vector<int> netIndices;
    netIndices.reserve(nets.size());
    for (const auto& net : nets)
        netIndices.push_back(net.getIndex());

    // Stage 1: Pattern routing
    n1 = netIndices.size();
    PatternRoute::readFluteLUT();
    cout << "stage 1: pattern routing" << '\n';

    vector<vector<int>> nonoverlapNetIndices;
    nonoverlapNetIndices.resize(5);
    int threshold = (gridGraph.getSize(0) + gridGraph.getSize(1)) / 2;
    separateNetIndices(netIndices, nonoverlapNetIndices, threshold);

#pragma omp parallel for
    for (int i = 0; i < 5; ++i) {
        sortNetIndices(nonoverlapNetIndices[i]);
        cout << "thread " << i << " size: " << nonoverlapNetIndices[i].size() << std::endl;
    }
    // sortNetIndices(netIndices);

    omp_lock_t lock;
    omp_init_lock(&lock);

    // the rest (smaller nets)
    // for (int i = 0; i < nonoverlapNetIndices[4].size(); ++i) {
    //     int netIndex = nonoverlapNetIndices[parameters.threads][i];
    //     PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
    //     patternRoute.constructSteinerTree();
    //     patternRoute.constructRoutingDAG();
    //     patternRoute.run();
    //     std::shared_ptr<GRTreeNode> tree = nets[netIndex].getRoutingTree();
    //     gridGraph.commitTree(tree);
    // }
    

#pragma omp parallel for
    for (int i = 0; i < parameters.threads; ++i) {
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
    auto t1 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    cout << "parallel for time elapsed: " << t1 << '\n';
    auto t_1 = std::chrono::high_resolution_clock::now();

    // the rest (larger nets)
    for (int i = 0; i < nonoverlapNetIndices[4].size(); ++i) {
        int netIndex = nonoverlapNetIndices[4][i];
        PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
        patternRoute.constructSteinerTree();
        patternRoute.constructRoutingDAG();
        patternRoute.run();
        std::shared_ptr<GRTreeNode> tree = nets[netIndex].getRoutingTree();
        gridGraph.commitTree(tree);
    }
    auto t2 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t_1).count();
    cout << "non-parallel for time elapsed: " << t2 << '\n';

    // for (const int netIndex : netIndices) {
    //     PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
    //     // if(netIndex == 577)
    //     //     cout << "id: " << netIndex << " debug1" << '\n';
    //     patternRoute.constructSteinerTree();
    //     patternRoute.constructRoutingDAG();
    //     patternRoute.run();
    //     // gridGraph.commitTree(nets[netIndex].getRoutingTree());
    //     std::shared_ptr<GRTreeNode> tree = nets[netIndex].getRoutingTree();
    //     gridGraph.commitTree(tree);
    //     // if(netIndex == 577)
    //     //     cout << "id: " << netIndex << " debug2" << '\n';
    // }

    // netIndices.clear();
    // for (const auto& net : nets) {
    //     if (gridGraph.checkOverflow(net.getRoutingTree(), 1) > 0) {
    //         netIndices.push_back(net.getIndex());
    //     }
    // }
    // cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << '\n';

    // t1 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    // t = std::chrono::high_resolution_clock::now();

    // cout << "stage 1 time elapsed: " << t1 << '\n';

    // for (const auto& netidx : netIndices)
    //     if(nets[netidx].name == "tcdm_slave_northeast_resp_o[12]")
    //         cout << "===== start stage 2 =====" << '\n';

    // Stage 2: Pattern routing with possible detours
    // n2 = netIndices.size();
    // if (netIndices.size() > 0) {
    //     cout << "stage 2: pattern routing with possible detours" << '\n';
    //     GridGraphView<bool> congestionView; // (2d) direction -> x -> y -> has overflow?
    //     gridGraph.extractCongestionView(congestionView);

    //     sortNetIndices(netIndices);
    //     for (const int netIndex : netIndices) {
    //         GRNet& net = nets[netIndex];
    //         gridGraph.commitTree(net.getRoutingTree(), true);
    //         PatternRoute patternRoute(net, gridGraph, parameters);
    //         patternRoute.constructSteinerTree();
    //         patternRoute.constructRoutingDAG();
    //         patternRoute.constructDetours(congestionView); // KEY DIFFERENCE compared to stage 1
    //         patternRoute.run();
    //         gridGraph.commitTree(net.getRoutingTree());
    //     }

    //     netIndices.clear();
    //     for (const auto& net : nets) {
    //         // if(net.name == "tcdm_slave_east_req_ready_o")
    //         //     cout << "id: " << net.getIndex() << " debug3" << '\n';
    //         if (gridGraph.checkOverflow(net.getRoutingTree(), 2) > 0) {
    //             netIndices.push_back(net.getIndex());
    //         }
    //     }
    //     cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << '\n';
    // }
    // t2 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    // t = std::chrono::high_resolution_clock::now();

    // cout << "stage 2 time elapsed: " << t2 << '\n';

    // for (const auto& netidx : netIndices)
    //     if(nets[netidx].name == "tcdm_slave_east_req_ready_o")
    //         cout << "===== start stage 3 =====" << '\n';

    // // Stage 3: maze routing on sparsified routing graph
    // n3 = netIndices.size();
    // if (netIndices.size() > 0) {
    //     cout << "stage 3: maze routing on sparsified routing graph" << '\n';
    //     for (const int netIndex : netIndices) {
    //         GRNet& net = nets[netIndex];
    //         gridGraph.commitTree(net.getRoutingTree(), true);
    //     }
    //     GridGraphView<CostT> wireCostView;
    //     gridGraph.extractWireCostView(wireCostView);
    //     sortNetIndices(netIndices);
    //     SparseGrid grid(10, 10, 0, 0);
    //     for (const int netIndex : netIndices) {
    //         // if(nets[netIndex].name == "tcdm_slave_northeast_resp_o[12]")
    //         //     cout << "id: " << netIndex << " debug3" << '\n';
    //         GRNet& net = nets[netIndex];
    //         // gridGraph.commitTree(net.getRoutingTree(), true);
    //         // gridGraph.updateWireCostView(wireCostView, net.getRoutingTree());
    //         MazeRoute mazeRoute(net, gridGraph, parameters);
    //         mazeRoute.constructSparsifiedGraph(wireCostView, grid);
    //         mazeRoute.run();
    //         std::shared_ptr<SteinerTreeNode> tree = mazeRoute.getSteinerTree();
    //         assert(tree != nullptr);

    //         PatternRoute patternRoute(net, gridGraph, parameters);
    //         patternRoute.setSteinerTree(tree);
    //         patternRoute.constructRoutingDAG();
    //         patternRoute.run();

    //         gridGraph.commitTree(net.getRoutingTree());
    //         gridGraph.updateWireCostView(wireCostView, net.getRoutingTree());
    //         grid.step();
    //     }
    //     netIndices.clear();
    //     for (const auto& net : nets) {
    //         if (gridGraph.checkOverflow(net.getRoutingTree(), 3) > 0) {
    //             netIndices.push_back(net.getIndex());
    //         }
    //     }
    //     cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << '\n';
    // }

    // t3 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    // t = std::chrono::high_resolution_clock::now();

    // cout << "stage 1: " << n1 << " nets, " << std::setprecision(3) << std::fixed << t1 << " seconds" << '\n';
    // cout << "stage 2: " << n2 << " nets, " << std::setprecision(3) << std::fixed << t2 << " seconds" << '\n';
    // cout << "stage 3: " << n3 << " nets, " << std::setprecision(3) << std::fixed << t3 << " seconds" << '\n';

    printStatistics();

    // if (parameters.write_heatmap) gridGraph.write();
}

void GlobalRouter::separateNetIndices(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices, int threshold) const {  // separate nets such that nets are routed in parallel
    // separate nets into 6 groups, up-left, up-right, down-left, down-right, the rest (smaller nets), the rest (larger nets)
    for (int netIndex : netIndices) {
        const GRNet& net = nets[netIndex];
        const auto& boundingBox = net.getBoundingBox();
        int xhigh = boundingBox.x.high;
        int xlow = boundingBox.x.low;
        int yhigh = boundingBox.y.high;
        int ylow = boundingBox.y.low;
        if (xhigh < gridGraph.getSize(0) / 2) {
            if (yhigh < gridGraph.getSize(1) / 2) {
                nonoverlapNetIndices[0].push_back(netIndex);
            } else if (ylow > gridGraph.getSize(1) / 2) {
                nonoverlapNetIndices[1].push_back(netIndex);
            } else {
                nonoverlapNetIndices[4].push_back(netIndex);
            }
        } else if (xlow > gridGraph.getSize(0) / 2) {
            if (yhigh < gridGraph.getSize(1) / 2) {
                nonoverlapNetIndices[2].push_back(netIndex);
            } else if (ylow > gridGraph.getSize(1) / 2) {
                nonoverlapNetIndices[3].push_back(netIndex);
            } else {
                nonoverlapNetIndices[4].push_back(netIndex);
            }
        } else {
            nonoverlapNetIndices[4].push_back(netIndex);
            // if (xhigh - xlow > threshold || yhigh - ylow > threshold) {
            //     nonoverlapNetIndices[4].push_back(netIndex);
            // } else {
            //     nonoverlapNetIndices[5].push_back(netIndex);
            // }
        }
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

void GlobalRouter::getGuides(const GRNet& net, vector<std::pair<int, utils::BoxT<int>>>& guides) {
    auto& routingTree = net.getRoutingTree();
    if (!routingTree)
        return;
    // 0. Basic guides
    GRTreeNode::preorder(routingTree, [&](std::shared_ptr<GRTreeNode> node) {
        for (const auto& child : node->children) {
            if (node->layerIdx == child->layerIdx) {
                guides.emplace_back(
                    node->layerIdx, utils::BoxT<int>(
                                        min(node->x, child->x), min(node->y, child->y),
                                        max(node->x, child->x), max(node->y, child->y)));
            } else {
                int maxLayerIndex = max(node->layerIdx, child->layerIdx);
                for (int layerIdx = min(node->layerIdx, child->layerIdx); layerIdx <= maxLayerIndex; layerIdx++) {
                    guides.emplace_back(layerIdx, utils::BoxT<int>(node->x, node->y));
                }
            }

            // if(node->layerIdx == 0)
            //     guides.emplace_back(node->layerIdx, utils::BoxT<int>(node->x, node->y));
        }
    });
}

void GlobalRouter::printStatistics() const {
    cout << "routing statistics" << '\n';

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

    cout << "wire length (metric):  " << wireLength << '\n';
    cout << "total via count:       " << viaCount << '\n';
    cout << "total wire overflow:   " << (int)overflow << '\n';

    cout << "min resource: " << minResource << '\n';
    cout << "bottleneck:   " << bottleneck << '\n';
}

void GlobalRouter::write(std::string guide_file) {
    cout << "generating route guides..." << '\n';
    if (guide_file == "")
        guide_file = parameters.out_file;

    // std::stringstream ss;
    FILE* out;
    out = fopen(guide_file.c_str(), "w");

    for (const GRNet& net : nets) {
        vector<std::pair<int, utils::BoxT<int>>> guides;
        getGuides(net, guides);
        // if(net.name == "tcdm_slave_northeast_resp_o[12]")
        //     cout << "id: " << net.getIndex() << " debug4" << '\n';

        fprintf(out, "%s\n(\n", net.getName().c_str());
        // ss << net.getName() << "\n";
        // ss << "(" << "\n";

        // --- modified by Alan ---
        std::map<std::pair<int, int>, std::pair<int, int>> verticalGuides;
        for (const auto& guide : guides) {
            // Store vertical guides to add vias later
            if (guide.second.x.low == guide.second.x.high && guide.second.y.low == guide.second.y.high) {
                auto key = std::make_pair(guide.second.x.low, guide.second.y.low);
                bool isExist = verticalGuides.find(key) != verticalGuides.end();

                if (isExist) {
                    auto& value = verticalGuides[key];
                    value.first = std::min(value.first, guide.first);    // find min layer
                    value.second = std::max(value.second, guide.first);  // find max layer
                } else {
                    verticalGuides[key] = std::make_pair(guide.first, guide.first);
                }
            } else {
                fprintf(out, "%d %d %d %d %d %d\n",
                        guide.second.x.low,
                        guide.second.y.low,
                        guide.first,
                        guide.second.x.high,
                        guide.second.y.high,
                        guide.first);
                // ss  << guide.second.x.low << " "
                //     << guide.second.y.low << " "
                //     << guide.first << " "
                //     << guide.second.x.high << " "
                //     << guide.second.y.high << " "
                //     << guide.first << "\n";
            }
        }

        // Add vias for vertical guides
        for (const auto& pair : verticalGuides) {
            // ss << pair.first.first << " "
            //    << pair.first.second << " "
            //    << pair.second.first << " "
            //    << pair.first.first << " "
            //    << pair.first.second << " "
            //    << pair.second.second << '\n';
            fprintf(out, "%d %d %d %d %d %d\n",
                    pair.first.first,
                    pair.first.second,
                    pair.second.first,
                    pair.first.first,
                    pair.first.second,
                    pair.second.second);
        }
        // ss << ")" << '\n';
        fprintf(out, ")\n");

        // --- end of modification ---
    }

    cout << '\n';
    cout << "writing output..." << '\n';
    // std::ofstream fout(guide_file);
    // fout << ss.str();
    // fout.close();
    cout << "finished writing output..." << '\n';
}
