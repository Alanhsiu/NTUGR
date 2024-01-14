// layer.h
#ifndef LAYER_H
#define LAYER_H

#include <vector>

using namespace std;

class Layer {
public:
    int id; 
    bool direction; // 0: horizontal, 1: vertical
    int minLength;
    vector<vector<int>> capacity; 
    
};

#endif // LAYER_H
