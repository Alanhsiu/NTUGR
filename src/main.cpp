#include <iostream>
// #include <experimental/filesystem>
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
    auto start = std::chrono::high_resolution_clock::now();
    cout << "GLOBAL ROUTING START" << std::endl;

    // Read input
    Parameters parameters(argc, argv);
    Design design(parameters);
    cout << "USED TIME FOR READ:" << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;
    
    // Global router
    GlobalRouter globalRouter(design, parameters);
    globalRouter.route();

    // Write result
    auto t_write = std::chrono::high_resolution_clock::now();
    globalRouter.write();
    cout << "USED TIME FOR WRITE:" << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - t_write).count() << std::endl;

    cout << "GLOBAL ROUTING END" << std::endl;
    cout << "TOTAL TIME:" << std::chrono::duration<double>(std::chrono::high_resolution_clock::now() - start).count() << std::endl;

    return 0;
}