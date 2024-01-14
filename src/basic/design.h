// design.h
#ifndef DESIGN_H
#define DESIGN_H

#include <string>
#include <vector>
#include "netlist.h"
#include "layer.h"
#include "dimension.h"
#include "metrics.h"

using namespace std;

class Design {
public:
    Design(const string& name) : name(name) {}

    string name;
    NetList netlist;
    vector<Layer> capacity;
    Dimension dimension;
    Metrics metrics;
};

#endif // DESIGN_H
