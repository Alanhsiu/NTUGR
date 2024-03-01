#pragma once
#include "../global.h"
#include "../basic/design.h"
#include "GridGraph.h"
#include "GRNet.h"

class GlobalRouter {
public:
    GlobalRouter(const Design& design, const Parameters& params);
    void route();
    void write(std::string guide_file = "");
    
// private:
    const Parameters& parameters;
    GridGraph gridGraph;
    vector<GRNet> nets;
    
    void separateNetIndices(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices) const;
    void separateNetIndices1(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices) const;
    void separateNetIndices2(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices) const;
    void separateNetIndices3(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices) const;
    void sortNetIndices(vector<int>& netIndices) const;
    void printStatistics() const;
};
