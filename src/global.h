#pragma once

#include "utils/utils.h"
using utils::log;
using utils::logeol;
using utils::loghline;
using utils::logmem;
using namespace std;

// STL libraries
#include <iostream>
#include <string>
#include <csignal>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <set>
#include <tuple>
#include <bitset>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <climits>
#include <iomanip>
#include <map>
#include <queue>
#include <typeinfo>
#include <omp.h>

#include "utils/robin_hood.h"

struct Parameters {
    std::string cap_file;
    std::string net_file;
    std::string out_file;
    int threads = 8;

    const int min_routing_layer = 1;
    const double max_detour_ratio = 0.25;
    const int target_detour_count = 20;
    const double via_multiplier = 1.5; // can change to 1.0, 1.5, 2.0 

    const double cost_logistic_slope1 = 1.5;
    const double cost_logistic_slope2 = 0.5;
    const double maze_logistic_slope = 0.5;
    const bool write_heatmap = false;

    const double UnitViaCost = 4.0;
    const double UnitViaDemand = 0.1;
    
    // command: /evaluator $input_path/$data.cap $input_path/$data.net $output_path/$data.PR_output
    Parameters(int argc, char* argv[]) {
        if (argc <= 1) {
            cout << "Too few args..." << '\n';
            exit(1);
        }
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-cap") == 0) {
                cap_file = argv[++i];
            } else if (strcmp(argv[i], "-net") == 0) {
                net_file = argv[++i];
            } else if (strcmp(argv[i], "-output") == 0) {
                out_file = argv[++i];
            // } else if (strcmp(argv[i], "-threads") == 0) {
            //     threads = std::stoi(argv[++i]);
            } else {
                cout << "Unrecognized arg..." << '\n';
                cout << argv[i] << '\n';
            }
        }
        cout << "cap file: " << cap_file << '\n';
        cout << "net file: " << net_file << '\n';
        cout << "output  : " << out_file << '\n';
        // cout << "threads : " << threads  << '\n';
        cout << '\n';
    }
};