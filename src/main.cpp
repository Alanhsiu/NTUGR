#include <iostream>
#include <experimental/filesystem>

#include "design.h"
#include "dimension.h"
#include "layer.h"
#include "metrics.h"
#include "netlist.h"

using namespace std;

int main(int argc, char* argv[]){
    // std::cout << "Current path is " << std::experimental::filesystem::current_path() << '\n';

    Design design("NTUGR");
    design.readNet("./input/ariane133_51.net");
    design.readCap("./input/ariane133_51.cap");
    cout << "read netlist and capacity done" << endl;
    return 0;
}