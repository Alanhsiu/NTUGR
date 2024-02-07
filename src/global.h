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
// #include <unordered_map>
// #include <unordered_set>
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

#include "utils/robin_hood.h"

struct Parameters {
    std::string cap_file;
    std::string net_file;
    std::string out_file;
    int threads = 1;

    const int min_routing_layer = 1;
    const double max_detour_ratio = 0.25;
    const int target_detour_count = 20;
    const double via_multiplier = 2.0;

    const double cost_logistic_slope = 1.0;
    const double maze_logistic_slope = 0.5;
    const bool write_heatmap = false;
    
    // command: /evaluator $input_path/$data.cap $input_path/$data.net $output_path/$data.PR_output
    Parameters(int argc, char* argv[]) {
        if (argc <= 1) {
            cout << "Too few args..." << endl;
            exit(1);
        }
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "-cap") == 0) {
                cap_file = argv[++i];
            } else if (strcmp(argv[i], "-net") == 0) {
                net_file = argv[++i];
            } else if (strcmp(argv[i], "-output") == 0) {
                out_file = argv[++i];
            } else if (strcmp(argv[i], "-threads") == 0) {
                threads = std::stoi(argv[++i]);
            } else {
                cout << "Unrecognized arg..." << endl;
                cout << argv[i] << endl;
            }
        }
        cout << "cap file: " << cap_file << endl;
        cout << "net file: " << net_file << endl;
        cout << "output  : " << out_file << endl;
        cout << "threads : " << threads  << endl;
        cout << endl;
    }
};