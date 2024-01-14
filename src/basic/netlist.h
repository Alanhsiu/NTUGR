// netlist.h
#ifndef NETLIST_H
#define NETLIST_H

#include <vector>
#include <string>

using namespace std;

class Point; 
class Pin;   
class Net;   

class NetList {
public:
    vector<Net> nets;
    vector<Pin> pins;
    vector<Point> points;
    int n_nets;
    int n_pins;
    int n_points;
    
};

class Net {
public:
    Net(int id, const string& name) : id(id), name(name) {}

    int id;
    string name;
    vector<int> pin_ids;
    
};

class Pin {
public:
    Pin(int id, int net_id) : id(id), net_id(net_id) {}

    int id;
    int net_id;
    vector<int> point_ids;
};

class Point {
public:
    Point(int id, int net_id, int layer, int x, int y) : id(id), net_id(net_id), layer(layer), x(x), y(y) {}
    
    int id;
    int net_id;
    int x;
    int y;
    int layer;
    
};

#endif // NETLIST_H
