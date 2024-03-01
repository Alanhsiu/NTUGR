// design.h
#ifndef DESIGN_H
#define DESIGN_H

#include <string>
#include <vector>

#include "../global.h"
#include "dimension.h"
#include "layer.h"
#include "metrics.h"
#include "netlist.h"
#include <chrono>

using namespace std;

class Design {
   public:
    Design(Parameters& params)
        : parameters(params) {
        auto t = std::chrono::high_resolution_clock::now();
        readCap(params.cap_file);
        cout << "USED TIME FOR READCAP:" <<  std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count() << std::endl;
        t = std::chrono::high_resolution_clock::now();
        readNet(params.net_file);
        cout << "USED TIME FOR READNET:" <<  std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count() << std::endl;
    }
    ~Design();

    // private:
    Parameters& parameters;
    // string name;
    NetList netlist;
    vector<Layer> layers;
    Dimension dimension;
    Metrics metrics;
    bool readCap(const string& filename);
    bool readNet(const string& filename);
};

#endif  // DESIGN_H
