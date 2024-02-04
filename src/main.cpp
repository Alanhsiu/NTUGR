#include <iostream>
#include <experimental/filesystem>
#include "global.h"
#include "design.h"
#include "dimension.h"
#include "layer.h"
#include "metrics.h"
#include "netlist.h"

extern "C" {
    #include "flute/flute.h"
}

using namespace std;

int main(int argc, char* argv[]){
    cout << "GLOBAL ROUTER CUGR" << std::endl;

    // Parse parameters
    Parameters parameters(argc, argv);
    Design design(parameters);
    cout << "read netlist and capacity done!" << endl;

    return 0;
}