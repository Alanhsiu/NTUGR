#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include "../basic/design.h"
#include "../basic/netlist.h"

using namespace std;

bool readNet(const string& filename, NetList& netlist) {
    ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        cerr << "Error opening the file." << endl;
        return false;
    }

    vector<Net> nets;
    vector<Pin> pins;
    vector<Point> points;

    string line;
    string net_name;

    int net_id = 0;
    int pin_id = 0;
    int point_id = 0;

    while (getline(inputFile, line)) {
        net_name = line;
        Net net(net_id++, net_name);
        vector<int> pin_ids;

        getline(inputFile, line);  // consumes the "("
        while (getline(inputFile, line)) {
            if (line == ")")
                break;  // end of net

            Pin pin(pin_id++, net_id);
            vector<int> point_ids;

            // Remove square brackets
            line.erase(remove(line.begin(), line.end(), '['), line.end());
            line.erase(remove(line.begin(), line.end(), ']'), line.end());
            istringstream iss(line);
            char comma;  // To handle commas between tuple elements

            while (!iss.eof()) {
                int layer, x, y;
                iss.ignore(1, '(');
                iss >> layer >> ws >> comma >> ws >> x >> ws >> comma >> ws >> y;
                iss.ignore(1, ')');
                iss >> comma >> ws;
                Point point(point_id++, net_id, layer, x, y);
                cout << "point_id: " << point.id << ", x: " << point.x << ", y: " << point.y << ", layer: " << point.layer << endl;
                point_ids.push_back(point.id);
                points.emplace_back(point);
            }

            pin.point_ids = point_ids;
            cout << "pin_id: " << pin.id << ", pin_size: " << pin.point_ids.size() << endl;
            pin_ids.push_back(pin.id);
            pins.emplace_back(pin);
        }
        net.pin_ids = pin_ids;
        nets.emplace_back(net);
    }

    netlist.n_nets = nets.size();
    netlist.n_pins = pins.size();
    netlist.n_points = points.size();
    netlist.nets = nets;
    netlist.pins = pins;
    netlist.points = points;

    return true;
}

// int main() {  // for testing
//     NetList netlist;
//     readNet("/Users/alanhsiu/Desktop/Github/NTUGR/input/example.net", netlist);
//     cout << "n_nets: " << netlist.n_nets << endl;
//     cout << "n_pins: " << netlist.n_pins << endl;
//     cout << "n_points: " << netlist.n_points << endl;
//     cout << "netlist: " << endl;
//     for (int i = 0; i < netlist.n_nets; i++) {
//         cout << "net_name: " << netlist.nets[i].name << ", net_id: " << netlist.nets[i].id << endl;
//         cout << "(" << endl;
//         for (int j = 0; j < netlist.nets[i].pin_ids.size(); j++) {
//             cout << "pin_id: " << netlist.pins[netlist.nets[i].pin_ids[j]].id << ", pin_size: " << netlist.pins[netlist.nets[i].pin_ids[j]].point_ids.size() << endl;
//             for(int k = 0; k < netlist.pins[netlist.nets[i].pin_ids[j]].point_ids.size(); k++){
//                 int point_id = netlist.pins[netlist.nets[i].pin_ids[j]].point_ids[k];
//                 cout << "point_id: " << netlist.points[point_id].id << ", x: " << netlist.points[point_id].x << ", y: " << netlist.points[point_id].y << ", layer: " << netlist.points[point_id].layer << endl;
//             }
//         }
//         cout << ")" << endl;
//     }
//     return 0;
// }