
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
    auto start = std::chrono::high_resolution_clock::now();

    vector<int> netIndices;
    netIndices.reserve(nets.size());
    cout << "nets size: " << nets.size() << std::endl;
    for (const auto& net : nets)
        netIndices.emplace_back(net.getIndex());

    // Stage 1: Pattern routing
    n1 = netIndices.size();
    PatternRoute::readFluteLUT();
    cout << "stage 1: pattern routing" << std::endl;

    int threadNum = parameters.threads;
    vector<vector<int>> nonoverlapNetIndices;
    nonoverlapNetIndices.resize(threadNum + 1);  // 8 for 8 threads, 1 for the rest

    /* Separate 1 */
    separateNetIndices3(netIndices, nonoverlapNetIndices);
#pragma omp parallel for
    for (int i = 0; i < nonoverlapNetIndices.size(); ++i) {
        sortNetIndices(nonoverlapNetIndices[i]);
    }
    for (int i = 0; i < nonoverlapNetIndices.size(); ++i)
        cout << "thread " << i << " size: " << nonoverlapNetIndices[i].size() << std::endl;

    /* Non-parallel 1 */
    int k = nonoverlapNetIndices[threadNum].size() / 2;
    for (int i = 0; i < k; ++i) {
        int netIndex = nonoverlapNetIndices[threadNum][i];
        PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
        patternRoute.constructSteinerTree();
        patternRoute.constructRoutingDAG();
        patternRoute.run();
        std::shared_ptr<GRTreeNode> tree = nets[netIndex].getRoutingTree();
        gridGraph.commitTree(tree);
    }
    cout << "non-parallel for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    start = std::chrono::high_resolution_clock::now();


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
    cout << "parallel 1 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    start = std::chrono::high_resolution_clock::now();

    /* Parallel 2 */
    //     netIndices.clear();
    //     netIndices.reserve(nonoverlapNetIndices[threadNum].size());
    //     for (int i = 0; i < nonoverlapNetIndices[threadNum].size(); ++i) {
    //         netIndices.push_back(nonoverlapNetIndices[threadNum][i]);
    //     }
    //     nonoverlapNetIndices[threadNum].clear();
    //     separateNetIndices(netIndices, nonoverlapNetIndices);

    // #pragma omp parallel for
    //     for (int i = 0; i < nonoverlapNetIndices.size(); ++i) {
    //         sortNetIndices(nonoverlapNetIndices[i]);
    //     }
    //     for (int i = 0; i < nonoverlapNetIndices.size(); ++i)
    //         cout << "thread " << i << " size: " << nonoverlapNetIndices[i].size() << std::endl;

    // #pragma omp parallel for
    //     for (int i = 0; i < threadNum; ++i) {
    //         for (int j = 0; j < nonoverlapNetIndices[i].size(); ++j) {
    //             int netIndex = nonoverlapNetIndices[i][j];
    //             PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
    //             omp_set_lock(&lock);
    //             patternRoute.constructSteinerTree();
    //             omp_unset_lock(&lock);
    //             patternRoute.constructRoutingDAG();
    //             patternRoute.run();
    //             std::shared_ptr<GRTreeNode> tree = nets[netIndex].getRoutingTree();
    //             gridGraph.commitTree(tree);
    //         }
    //     }
    //     cout << "parallel 2 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    //     start = std::chrono::high_resolution_clock::now();

    /* Non-parallel 1 */
    for (int i = k; i < nonoverlapNetIndices[threadNum].size(); ++i) {
        int netIndex = nonoverlapNetIndices[threadNum][i];
        PatternRoute patternRoute(nets[netIndex], gridGraph, parameters);
        patternRoute.constructSteinerTree();
        patternRoute.constructRoutingDAG();
        patternRoute.run();
        std::shared_ptr<GRTreeNode> tree = nets[netIndex].getRoutingTree();
        gridGraph.commitTree(tree);
    }
    cout << "non-parallel for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    start = std::chrono::high_resolution_clock::now();

    netIndices.clear();
    for (const auto& net : nets) {
        if (gridGraph.checkOverflow(net.getRoutingTree(), 1) > 0) {
            netIndices.push_back(net.getIndex());
        }
    }
    cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << std::endl;

    // Stage 2: Pattern routing with possible detours
    if (parameters.stage2 && netIndices.size() > 0) {
        n2 = netIndices.size();
        cout << "stage 2: pattern routing with possible detours" << std::endl;
        GridGraphView<bool> congestionView;  // (2d) direction -> x -> y -> has overflow?
        gridGraph.extractCongestionView(congestionView);

        // sortNetIndices(netIndices);
        // for (const int netIndex : netIndices) {
        //     GRNet& net = nets[netIndex];
        //     gridGraph.commitTree(net.getRoutingTree(), true);
        //     PatternRoute patternRoute(net, gridGraph, parameters);
        //     patternRoute.constructSteinerTree();
        //     patternRoute.constructRoutingDAG();
        //     patternRoute.constructDetours(congestionView);  // KEY DIFFERENCE compared to stage 1
        //     patternRoute.run();
        //     gridGraph.commitTree(net.getRoutingTree());
        // }

        vector<vector<int>> nonoverlapNetIndices;
        nonoverlapNetIndices.resize(threadNum + 1);  // 8 for 8 threads, 1 for the rest

        /* Separate 2 */
        separateNetIndices3(netIndices, nonoverlapNetIndices);
#pragma omp parallel for
        for (int i = 0; i < nonoverlapNetIndices.size(); ++i) {
            sortNetIndices(nonoverlapNetIndices[i]);
        }
        for (int i = 0; i < nonoverlapNetIndices.size(); ++i)
            cout << "thread " << i << " size: " << nonoverlapNetIndices[i].size() << std::endl;

        /* Non-parallel 1 */
        // int k = 0;
        int k = nonoverlapNetIndices[threadNum].size() / 2;
        for (int i = 0; i < k; ++i) {
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
        cout << "non-parallel for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
        start = std::chrono::high_resolution_clock::now();

        /* Parallel 1 */
        omp_lock_t lock;
        omp_init_lock(&lock);

#pragma omp parallel for
        for (int i = 0; i < threadNum; ++i) {
            for (int j = 0; j < nonoverlapNetIndices[i].size(); ++j) {
                int netIndex = nonoverlapNetIndices[i][j];
                GRNet& net = nets[netIndex];
                gridGraph.commitTree(net.getRoutingTree(), true);
                PatternRoute patternRoute(net, gridGraph, parameters);
                patternRoute.constructSteinerTree();
                patternRoute.constructRoutingDAG();
                patternRoute.constructDetours(congestionView);  // KEY DIFFERENCE compared to stage 1
                patternRoute.run();
                gridGraph.commitTree(net.getRoutingTree());
            }
        }
        cout << "parallel 1 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
        start = std::chrono::high_resolution_clock::now();

        /* Non-parallel 1 */
        for (int i = k; i < nonoverlapNetIndices[threadNum].size(); ++i) {
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
        cout << "non-parallel for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
        start = std::chrono::high_resolution_clock::now();

        netIndices.clear();
        for (const auto& net : nets) {
            if (gridGraph.checkOverflow(net.getRoutingTree(), 2) > 0) {
                netIndices.push_back(net.getIndex());
            }
        }
        cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << std::endl;

        cout << "stage 2 time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
        start = std::chrono::high_resolution_clock::now();
    }

    // // Stage 3: maze routing on sparsified routing graph
    // n3 = netIndices.size();
    // if (netIndices.size() > 0) {
    //     cout << "stage 3: maze routing on sparsified routing graph" << std::endl;
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
    //         //     cout << "id: " << netIndex << " debug3" << std::endl;
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
    //     cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << std::endl;
    // }

    // t3 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    // t = std::chrono::high_resolution_clock::now();

    // cout << "stage 1: " << n1 << " nets, " << std::setprecision(3) << std::fixed << t1 << " seconds" << std::endl;
    // cout << "stage 2: " << n2 << " nets, " << std::setprecision(3) << std::fixed << t2 << " seconds" << std::endl;
    // cout << "stage 3: " << n3 << " nets, " << std::setprecision(3) << std::fixed << t3 << " seconds" << std::endl;

    printStatistics();

    // if (parameters.write_heatmap) gridGraph.write();
}

void GlobalRouter::separateNetIndices(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices) const {  // separate nets such that nets are routed in parallel
    // separate nets into 8 groups, and the rest
    int netSize = netIndices.size();
    for (int netIndex : netIndices) {
        const GRNet& net = nets[netIndex];
        const auto& boundingBox = net.getBoundingBox();

        int xlow = boundingBox.x.low, xhigh = boundingBox.x.high, ylow = boundingBox.y.low, yhigh = boundingBox.y.high;
        int xSize = gridGraph.getSize(0), ySize = gridGraph.getSize(1);

        if (xhigh < xSize / 2) {
            if (yhigh < ySize / 2) {
                if (yhigh < ySize / 4 && nonoverlapNetIndices[0].size() < netSize / 4) {
                    nonoverlapNetIndices[0].push_back(netIndex);
                    continue;
                } else if (ylow > ySize / 4 && nonoverlapNetIndices[1].size() < netSize / 4) {
                    nonoverlapNetIndices[1].push_back(netIndex);
                    continue;
                }
            } else if (ylow > ySize / 2) {
                if (yhigh < 3 * ySize / 4 && nonoverlapNetIndices[2].size() < netSize / 4) {
                    nonoverlapNetIndices[2].push_back(netIndex);
                    continue;
                } else if (ylow > 3 * ySize / 4 && nonoverlapNetIndices[3].size() < netSize / 4) {
                    nonoverlapNetIndices[3].push_back(netIndex);
                    continue;
                }
            }
        } else if (xlow > xSize / 2) {
            if (yhigh < ySize / 2) {
                if (yhigh < ySize / 4 && nonoverlapNetIndices[4].size() < netSize / 4) {
                    nonoverlapNetIndices[4].push_back(netIndex);
                    continue;
                } else if (ylow > ySize / 4 && nonoverlapNetIndices[5].size() < netSize / 4) {
                    nonoverlapNetIndices[5].push_back(netIndex);
                    continue;
                }
            } else if (ylow > ySize / 2) {
                if (yhigh < 3 * ySize / 4 && nonoverlapNetIndices[6].size() < netSize / 4) {
                    nonoverlapNetIndices[6].push_back(netIndex);
                    continue;
                } else if (ylow > 3 * ySize / 4 && nonoverlapNetIndices[7].size() < netSize / 4) {
                    nonoverlapNetIndices[7].push_back(netIndex);
                    continue;
                }
            }
        }
        nonoverlapNetIndices[8].push_back(netIndex);
    }
}

void GlobalRouter::separateNetIndices2(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices) const {  // separate nets such that nets are routed in parallel
    // separate nets into 8 groups, and the rest
    int netSize = netIndices.size();
    for (int netIndex : netIndices) {
        const GRNet& net = nets[netIndex];
        const auto& boundingBox = net.getBoundingBox();

        int xlow = boundingBox.x.low, xhigh = boundingBox.x.high, ylow = boundingBox.y.low, yhigh = boundingBox.y.high;
        int xSize = gridGraph.getSize(0), ySize = gridGraph.getSize(1);

        // 6 divide lines, x=1/4, 3/4, y=1/8, 3/8, 5/8, 7/8
        if ((xhigh < 1 / 4) || (xlow > 3 / 4)) {
            if ((yhigh < 1 / 8) || (ylow > 7 / 8)) {
                nonoverlapNetIndices[0].push_back(netIndex);
                continue;

            } else if ((yhigh < 3 / 8) || (ylow > 1 / 8)) {
                nonoverlapNetIndices[1].push_back(netIndex);
                continue;

            } else if ((yhigh < 5 / 8) || (ylow > 3 / 8)) {
                nonoverlapNetIndices[2].push_back(netIndex);
                continue;

            } else if ((yhigh < 7 / 8) || (ylow > 5 / 8)) {
                nonoverlapNetIndices[3].push_back(netIndex);
                continue;
            }
        } else if ((xhigh < 3 / 4) || (xlow > 1 / 4)) {
            if ((yhigh < 1 / 8) || (ylow > 7 / 8)) {
                nonoverlapNetIndices[4].push_back(netIndex);
                continue;

            } else if ((yhigh < 3 / 8) || (ylow > 1 / 8)) {
                nonoverlapNetIndices[5].push_back(netIndex);
                continue;

            } else if ((yhigh < 5 / 8) || (ylow > 3 / 8)) {
                nonoverlapNetIndices[6].push_back(netIndex);
                continue;

            } else if ((yhigh < 7 / 8) || (ylow > 5 / 8)) {
                nonoverlapNetIndices[7].push_back(netIndex);
                continue;
            }
        }
        nonoverlapNetIndices[8].push_back(netIndex);
    }
}

void GlobalRouter::separateNetIndices3(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices) const {  // separate nets such that nets are routed in parallel
    // separate nets into 9 groups, and the rest
    int netSize = netIndices.size();
    for (int netIndex : netIndices) {
        const GRNet& net = nets[netIndex];
        const auto& boundingBox = net.getBoundingBox();

        int xlow = boundingBox.x.low, xhigh = boundingBox.x.high, ylow = boundingBox.y.low, yhigh = boundingBox.y.high;
        int xSize = gridGraph.getSize(0), ySize = gridGraph.getSize(1);

        if (xhigh < xSize / 3) {
            if (yhigh < ySize / 3) {
                nonoverlapNetIndices[0].push_back(netIndex);
                continue;
            } else if (ylow > 2 * ySize / 3) {
                nonoverlapNetIndices[1].push_back(netIndex);
                continue;
            } else if (yhigh < 2 * ySize / 3 && ylow > ySize / 3) {
                nonoverlapNetIndices[2].push_back(netIndex);
                continue;
            }
        } else if (xlow > 2 * xSize / 3) {
            if (yhigh < ySize / 3) {
                nonoverlapNetIndices[3].push_back(netIndex);
                continue;
            } else if (ylow > 2 * ySize / 3) {
                nonoverlapNetIndices[4].push_back(netIndex);
                continue;
            } else if (yhigh < 2 * ySize / 3 && ylow > ySize / 3) {
                nonoverlapNetIndices[5].push_back(netIndex);
                continue;
            }
        } else {
            if (yhigh < ySize / 3) {
                nonoverlapNetIndices[6].push_back(netIndex);
                continue;
            } else if (ylow > 2 * ySize / 3) {
                nonoverlapNetIndices[7].push_back(netIndex);
                continue;
            }
        }
        nonoverlapNetIndices[8].push_back(netIndex);
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

#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

void write_partial_file(const std::vector<GRNet>& nets_subset, const std::string& temp_filename) {
    std::ofstream out(temp_filename);
    for (const GRNet& net : nets_subset) {
        out << net.guide_string << "\n";
    }
}

void parallel_write_to_file(const std::vector<GRNet>& nets, const std::string& guide_file, int num_threads) {
    std::vector<std::thread> threads;
    std::vector<std::string> temp_files(num_threads);

    // 分割 nets 到多個子集
    int subset_size = nets.size() / num_threads;
    for (int i = 0; i < num_threads; ++i) {
        int start = i * subset_size;
        int end = (i + 1) == num_threads ? nets.size() : (i + 1) * subset_size;
        std::vector<GRNet> subset(nets.begin() + start, nets.begin() + end);
        temp_files[i] = "temp_" + std::to_string(i) + ".txt";
        threads.push_back(std::thread(write_partial_file, subset, temp_files[i]));
    }

    // 等待所有線程完成
    for (auto& t : threads) {
        t.join();
    }

    // 合併臨時文件到最終文件
    std::ofstream final_out(guide_file);
    for (const auto& temp_file : temp_files) {
        std::ifstream temp_in(temp_file);
        final_out << temp_in.rdbuf();
        // 刪除臨時文件
        std::remove(temp_file.c_str());
    }
}

void GlobalRouter::write(std::string guide_file) {
    cout << "generating route guides..." << std::endl;
    if (guide_file == "")
        guide_file = parameters.out_file;

    int netSize = nets.size();

    // record time
    auto start = std::chrono::high_resolution_clock::now();
#pragma omp parallel for
    for (int i = 0; i < netSize; i++) {
        nets[i].getGuides();
    }
    // print time
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Elapsed time for getGuides: " << elapsed.count() << " s" << endl;

    // FILE* out;
    // out = fopen(guide_file.c_str(), "w");
    // for (const GRNet& net : nets) {
    //     fprintf(out, "%s\n(\n", net.getName().c_str());
    //     for (const auto& g : net.guide) {
    //         fprintf(out, "%d %d %d %d %d %d\n",
    //                 g[0], g[1], g[2], g[3], g[4], g[5]);
    //     }
    //     fprintf(out, ")\n");
    // }

    FILE* out;
    out = fopen(guide_file.c_str(), "w");
    for (const GRNet& net : nets) {
        fprintf(out, "%s\n", net.guide_string.c_str());
    }

    // parallel_write_to_file(nets, guide_file, parameters.threads);

    cout << std::endl;
    // cout << "writing output..." << std::endl;
    fclose(out);
    cout << "finished writing output..." << std::endl;
}