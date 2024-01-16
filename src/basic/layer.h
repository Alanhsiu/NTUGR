// layer.h
#ifndef LAYER_H
#define LAYER_H

#include <vector>

using namespace std;

class Layer {
public:
    int id; 
    bool direction; // 0: horizontal, 1: vertical
    double minLength;
    vector<vector<double>> capacity; 
    
};

#endif // LAYER_H
