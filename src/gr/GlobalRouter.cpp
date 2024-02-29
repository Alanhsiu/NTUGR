
#include "GlobalRouter.h"
#include "PatternRoute.h"
// #include "MazeRoute.h"
#include <chrono>

GlobalRouter::GlobalRouter(const Design& design, const Parameters& params)
    : gridGraph(design, params), parameters(params) {
    // // Instantiate the global routing netlist
    // const vector<Net>& baseNets = design.netlist.nets;
    // nets.reserve(baseNets.size());
    // for (const Net& baseNet : baseNets) {
    //     nets.emplace_back(baseNet, design, gridGraph);  // GRNet(baseNet, design, gridGraph)
    // }
    const size_t numNets = design.netlist.nets.size();
    nets.reserve(numNets);
    for (const Net& baseNet : design.netlist.nets) {
        nets.emplace_back(baseNet, design, gridGraph);
    }
}

#include <unistd.h>
void monitorMemoryUsage() {
    // 取得目前進程 ID
    pid_t pid = getpid();

    // 構建 /proc/<pid>/status 檔案的路徑
    std::string statusFilePath = "/proc/" + std::to_string(pid) + "/status";

    // 開啟 /proc/<pid>/status 檔案
    std::ifstream statusFile(statusFilePath);
    std::string line;

    // 在檔案中尋找 VmRSS（實際使用的記憶體量）
    while (std::getline(statusFile, line)) {
        if (line.find("VmRSS") != std::string::npos) {
            std::cout << "Memory usage: " << line << std::endl;
            break;
        }
    }
}

void GlobalRouter::route() {
    int n1 = 0, n2 = 0;  // for statistics
    auto t = std::chrono::high_resolution_clock::now();
    auto start = std::chrono::high_resolution_clock::now();

    vector<int> netIndices;
    netIndices.reserve(nets.size());
    for (const auto& net : nets)
        netIndices.emplace_back(net.getIndex());
    // int overflowThreshold = (nets.size() < 150000) ? 0 : -10;
    // int overflowThreshold = (nets.size() < 10000000) ? -4 : -8;
    // int overflowThreshold = -3;
    // if (nets.size() > 10000000) {
    //     overflowThreshold = -7;
    // } else if (nets.size() < 130000) {
    //     overflowThreshold = -1;
    // }
    int overflowThreshold = (nets.size() < 1000000) ? -4 : -10;
    cout << "nets size: " << nets.size() << ", overflowThreshold: " << overflowThreshold << std::endl;
monitorMemoryUsage();
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
monitorMemoryUsage();
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
monitorMemoryUsage();
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
monitorMemoryUsage();
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
        if (gridGraph.checkOverflow(net.getRoutingTree(), overflowThreshold) > 0) {
            netIndices.push_back(net.getIndex());
        }
    }
    cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << std::endl;

    // Stage 2: Pattern routing with possible detours
    auto t2 = std::chrono::high_resolution_clock::now();
    if (parameters.stage2 && netIndices.size() > 0) {
        n2 = netIndices.size();
        cout << "stage 2: pattern routing with possible detours" << std::endl;
        GridGraphView<bool> congestionView;  // (2d) direction -> x -> y -> has overflow?
        gridGraph.extractCongestionView(congestionView);
monitorMemoryUsage();
        /* Separate 2 */
        vector<vector<int>> nonoverlapNetIndices;
        nonoverlapNetIndices.resize(threadNum + 1);  // 8 for 8 threads, 1 for the rest
        separateNetIndices(netIndices, nonoverlapNetIndices);
#pragma omp parallel for
        for (int i = 0; i < nonoverlapNetIndices.size(); ++i) {
            sortNetIndices(nonoverlapNetIndices[i]);
        }
        for (int i = 0; i < nonoverlapNetIndices.size(); ++i)
            cout << "thread " << i << " size: " << nonoverlapNetIndices[i].size() << std::endl;
monitorMemoryUsage();
        /* Parallel 1 */
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
        cout << "parallel 1 for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
        start = std::chrono::high_resolution_clock::now();
monitorMemoryUsage();
        /* Non-parallel 1 */
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
        cout << "non-parallel for time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
        start = std::chrono::high_resolution_clock::now();

        netIndices.clear();
        for (const auto& net : nets) {
            if (gridGraph.checkOverflow(net.getRoutingTree(), overflowThreshold) > 0) {
                netIndices.push_back(net.getIndex());
            }
        }
        cout << netIndices.size() << " / " << nets.size() << " nets have overflows." << std::endl;

        cout << "stage 2 time elapsed: " << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t2).count() << std::endl;
        start = std::chrono::high_resolution_clock::now();
    }

    printStatistics();
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
        out << net.guide_string;
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

#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

// 假設GRNet已經被定義，並且有一個成員變量guide_string

void write_subset_to_file(const std::vector<GRNet>& subset, const std::string& filename) {
    std::ofstream out(filename);
    for (const auto& net : subset) {
        out << net.guide_string;
    }
}

void parallel_write_and_merge(const std::vector<GRNet>& nets, const std::string& final_filename, int num_threads) {
    std::vector<std::thread> threads;
    std::vector<std::string> temp_filenames(num_threads);

    // 分割 nets 到多個子集並且平行寫入到臨時檔案
    int subset_size = nets.size() / num_threads;
    for (int i = 0; i < num_threads; ++i) {
        int start = i * subset_size;
        int end = (i + 1) == num_threads ? nets.size() : (i + 1) * subset_size;
        temp_filenames[i] = "temp_" + std::to_string(i) + ".txt";
        std::vector<GRNet> subset(nets.begin() + start, nets.begin() + end);
        threads.push_back(std::thread(write_subset_to_file, subset, temp_filenames[i]));
    }

    // 等待所有線程完成
    for (auto& thread : threads) {
        thread.join();
    }

    // 合併臨時檔案到最終檔案，使用更大的緩衝區
    std::ofstream final_out(final_filename, std::ios::binary);
    const size_t buffer_size = 1048576;  // 1MB
    std::vector<char> buffer(buffer_size);
    for (const auto& temp_filename : temp_filenames) {
        std::ifstream temp_in(temp_filename, std::ios::binary);
        while (temp_in.read(buffer.data(), buffer.size())) {
            final_out.write(buffer.data(), temp_in.gcount());
        }
        if (temp_in.gcount() > 0) {  // 確保最後一次讀取的數據被寫入
            final_out.write(buffer.data(), temp_in.gcount());
        }
        std::remove(temp_filename.c_str());  // 刪除臨時文件
    }
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

    // parallel_write_to_file(nets, guide_file, parameters.threads); // -> 變很久

    // parallel_write_and_merge(nets, guide_file, 8); // -> 變很久

    cout << std::endl;
    cout << "finished writing output..." << std::endl;
}