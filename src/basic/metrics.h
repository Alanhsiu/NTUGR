// metrics.h
#ifndef METRICS_H
#define METRICS_H

#include <vector>

using namespace std;

class Metrics {
public:
    double wireCost;
    double viaCost;
    vector<double> OFWeight; 
    
};

#endif // METRICS_H
