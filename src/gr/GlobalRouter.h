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
    std::string cap_file_name = "/home/b09901066/ISPD-NTUEE/NTUGR/heatmaps/partial_cap.txt";
    
// private:
    const Parameters& parameters;
    GridGraph gridGraph;
    vector<GRNet> nets;
    
    void writeExtractNetToFile(vector<std::pair<Point, Point>>& extract_net, const string& filename) const;
    void separateNetIndices(vector<int>& netIndices, vector<vector<int>>& nonoverlapNetIndices) const;
    void sortNetIndices(vector<int>& netIndices) const;
    void printStatistics() const;
    void write_partial_cap(vector<vector<vector<double>>>& cap) const;
};