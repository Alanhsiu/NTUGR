// metrics.h
#ifndef METRICS_H
#define METRICS_H

#include <vector>

using namespace std;

class Metrics {
public:
    int wireCost;
    int viaCost;
    vector<vector<int>> ofWeight; 
    
};

#endif // METRICS_H
