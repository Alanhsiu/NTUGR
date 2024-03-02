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
    FILE* inputFile = fopen(filename.c_str(), "r");
    char line[1000];
    if (inputFile == NULL) {
        printf("Error opening file.\n");
        return false;
    }
    fscanf(inputFile, "%d %d %d", &dimension.n_layers, &dimension.x_size, &dimension.y_size);
    // fgets(line, sizeof(line), inputFile)
    fscanf(inputFile, "%lf %lf", &metrics.UnitLengthWireCost, &metrics.UnitViaCost);
    parameters.UnitViaCost = metrics.UnitViaCost;
    cout << "UnitViaCost: " << parameters.UnitViaCost << endl;
    double OFWeight;
    metrics.OFWeight.reserve(dimension.n_layers);
    for (int i = 0; i < dimension.n_layers; i++) {
        fscanf(inputFile, "%lf", &OFWeight);
        metrics.OFWeight.emplace_back(OFWeight);
    }
    dimension.hEdge.reserve(dimension.x_size);
    int hEdge;
    for (int i = 0; i < dimension.x_size-1; i++) {
        fscanf(inputFile, "%d", &hEdge);
        dimension.hEdge.emplace_back(hEdge);
    }
    dimension.vEdge.reserve(dimension.y_size);
    int vEdge;
    for (int i = 0; i < dimension.y_size-1; i++) {
        fscanf(inputFile, "%d", &vEdge);
        dimension.vEdge.emplace_back(vEdge);
    }
    char name[1000];
    for (int i = 0; i < dimension.n_layers; i++) {
        Layer layer;
        layer.id = i;
        fscanf(inputFile, "%s %d %lf", name, &layer.direction, &layer.minLength);
        vector<vector<double>> cap(dimension.y_size, vector<double>(dimension.x_size, 0));
        for (int i = 0; i < dimension.y_size; i++) {
            for (int j = 0; j < dimension.x_size; j++) {
                fscanf(inputFile, "%lf", &cap[i][j]);
            }
        }
        layer.capacity = cap;
        layers.push_back(layer);
    }
    fclose(inputFile);
    return true;
}

void replaceChars(char *str) {
    while (*str != '\0') {
        if (*str == '[' || *str == ']' || *str == '(' || *str == ')' || *str == ',') {
            *str = ' ';
        }
        str++;
    }
}

bool Design::readNet(const string& filename) {

    vector<Net> nets;
    vector<Pin> pins;
    vector<Point> points;

    char net_name[1000];

    int net_id = 0;
    int pin_id = 0;
    int point_id = 0;

    FILE* inputFile = fopen(filename.c_str(), "r");
    char line[1000];
    if (inputFile == NULL) {
        printf("Error opening file.\n");
        return false;
    }
    while (fgets(line, sizeof(line), inputFile)){
        size_t ln = strlen(line) - 1;
        if (line[ln] == '\n') line[ln] = '\0';
        string str(line);
        Net net(net_id++, str);
        vector<int> pin_ids;
        fgets(line, sizeof(line), inputFile); // consumes the "("
        while (fgets(line, sizeof(line), inputFile)) {
            int layer, x, y;
            if (strcmp(line, ")\n") == 0){
                break; // end of net
            }
            Pin pin(pin_id++, net_id);
            vector<int> point_ids;
            replaceChars(line);
            char *token = strtok(line, " \t\n");
            while (token != NULL) {
                layer = atoi(token);
                token = strtok(NULL, " \t\n");
                if (token != NULL) {
                    x = atoi(token);
                    token = strtok(NULL, " \t\n");
                } else {
                    break; 
                }
                if (token != NULL) {
                    y = atoi(token);
                    token = strtok(NULL, " \t\n");
                } else {
                    break;
                }
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
    fclose(inputFile);
    return true;
}
