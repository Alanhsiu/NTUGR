// dimension.h
#ifndef DIMENSION_H
#define DIMENSION_H

#include <vector>

using namespace std;

class Dimension {
public:
    int n_layers;
    int x_size;
    int y_size;
    vector<int> hEdge;
    vector<int> vEdge;
    
};

#endif // DIMENSION_H
