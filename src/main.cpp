#include <iostream>
#include <experimental/filesystem>
#include <chrono>
#include "global.h"
#include "../basic/design.h"
#include "../gr/GlobalRouter.h"

extern "C" {
    #include "flute/flute.h"
}

using namespace std;

int main(int argc, char* argv[]){
    auto t = std::chrono::high_resolution_clock::now();
    cout << "GLOBAL ROUTER CUGR" << std::endl;

    // Parse parameters
    Parameters parameters(argc, argv);
    Design design(parameters);
    cout << "read netlist and capacity done!" << endl;
    auto t1 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    cout << "USED TIME FOR READ:" << t1 << std::endl;

    // Global router
    GlobalRouter globalRouter(design, parameters);
    globalRouter.route();
    globalRouter.write();

    cout << "GLOBAL ROUTER CUGR DONE" << std::endl;
    auto t2 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    cout << "USED TIME:" << t2 << std::endl;

    return 0;
}