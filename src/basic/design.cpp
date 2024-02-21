#include "../basic/design.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include "../basic/netlist.h"

using namespace std;

Design::~Design() {
}

bool Design::readCap(const string& filename) {
    ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        cerr << "Error opening the file." << endl;
        return false;
    }
    string line;
    // nLayers, xSize, ySize
    getline(inputFile, line);
    istringstream iss1(line);
    iss1 >> dimension.n_layers >> dimension.x_size >> dimension.y_size;
    // cost
    getline(inputFile, line);
    istringstream iss2(line);
    iss2 >> metrics.UnitLengthWireCost >> metrics.UnitViaCost;
    double OFWeight;
    // metrics.OFWeight.resize(dimension.n_layers);
    for (int i = 0; i < dimension.n_layers; i++) {
        iss2 >> OFWeight;
        metrics.OFWeight.emplace_back(OFWeight);
    }
    // x step sizes
    // dimension.hEdge.resize(dimension.x_size);
    getline(inputFile, line);
    istringstream iss3(line);
    int hEdge;
    for (int i = 0; i < dimension.x_size-1; i++) {
        iss3 >> hEdge;
        dimension.hEdge.emplace_back(hEdge);
    }
    // y step sizes
    // dimension.vEdge.resize(dimension.y_size);
    getline(inputFile, line);
    istringstream iss4(line);
    int vEdge;
    for (int i = 0; i < dimension.y_size-1; i++) {
        iss4 >> vEdge;
        dimension.vEdge.emplace_back(vEdge);
    }

    // capacity
    string name;
    for (int i = 0; i < dimension.n_layers; i++) {
        Layer layer;
        layer.id = i;
        getline(inputFile, line);
        istringstream iss(line);
        iss >> name >> layer.direction >> layer.minLength;
        layer.minLength = layer.minLength;
        vector<vector<double>> cap(dimension.y_size, vector<double>(dimension.x_size, 0));
        for (int i = 0; i < dimension.y_size; i++) {
            getline(inputFile, line);
            istringstream iss(line);
            for (int j = 0; j < dimension.x_size; j++) {
                iss >> cap[i][j];
            }
        }
        layer.capacity = cap;
        layers.push_back(layer);
    }
    return true;
}

bool Design::readNet(const string& filename) {
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
                point_ids.push_back(point.id);
                points.emplace_back(point);
            }

            pin.point_ids = point_ids;
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
/* netlist testing */
// int main() {
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