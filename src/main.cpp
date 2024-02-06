#include <iostream>
#include <experimental/filesystem>
#include "global.h"
#include "../basic/design.h"
#include "../gr/GlobalRouter.h"

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

    // Global router
    GlobalRouter globalRouter(design, parameters);
    globalRouter.route();
    globalRouter.write();

    cout << "GLOBAL ROUTER CUGR DONE" << std::endl;

    return 0;
}