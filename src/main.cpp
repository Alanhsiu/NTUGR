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
    ios::sync_with_stdio(false);
    auto t = std::chrono::high_resolution_clock::now();
    cout << "GLOBAL ROUTER CUGR" << std::endl;

    // Parse parameters
    Parameters parameters(argc, argv);
    Design design(parameters);
    cout << "read netlist and capacity done!" << std::endl;
    auto t1 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    cout << "USED TIME FOR READ:" << t1 << std::endl;
    
    // Global router
    GlobalRouter globalRouter(design, parameters);
    globalRouter.route();
    auto t_write = std::chrono::high_resolution_clock::now();
    globalRouter.write();
    auto t2 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t_write).count();
    cout << "write: " << std::setprecision(3) << std::fixed << t2 << " seconds" << std::endl;

    cout << "GLOBAL ROUTER CUGR DONE" << std::endl;
    auto t3 = std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t).count();
    cout << "USED TIME:" << t3 << std::endl;

    return 0;
}