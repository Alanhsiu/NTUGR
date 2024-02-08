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

using namespace std;

class Design {
   public:
    Design(const Parameters& params)
        : parameters(params) {
        readCap(params.cap_file);
        readNet(params.net_file);
    }
    ~Design();

    // private:
    const Parameters& parameters;
    // string name;
    NetList netlist;
    vector<Layer> layers;
    Dimension dimension;
    Metrics metrics;
    bool readCap(const string& filename);
    bool readNet(const string& filename);
};

#endif  // DESIGN_H
