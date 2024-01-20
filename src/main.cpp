#include <iostream>
#include <experimental/filesystem>

#include "design.h"
#include "dimension.h"
#include "layer.h"
#include "metrics.h"
#include "netlist.h"
#include "flute/flute.h"

using namespace std;

int main(int argc, char* argv[]){
    // std::cout << "Current path is " << std::experimental::filesystem::current_path() << '\n';

    Design design("NTUGR");
    // design.readNet("./input/ariane133_51.net");
    // design.readCap("./input/ariane133_51.cap");
    cout << "read netlist and capacity done!" << endl;

    int d = 4; // 点的数量
    DTYPE x[] = {10, 20, 30, 40}; // X坐标数组
    DTYPE y[] = {10, 20, 30, 40}; // Y坐标数组

    // Tree t = flute(d, x, y, ACCURACY);

    return 0;
}