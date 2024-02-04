#pragma once
#include "../global.h"
#include "GRNet.h"
#include "../flute/flute.h"

extern "C" {
void readLUT();
}

class SteinerTreeNode: public utils::PointT<int> {
public:
    vector<std::shared_ptr<SteinerTreeNode>> children;
    utils::IntervalT<int> fixedLayers;
    
    SteinerTreeNode(utils::PointT<int> point): utils::PointT<int>(point) {}
    SteinerTreeNode(utils::PointT<int> point, utils::IntervalT<int> _fixedLayers): 
        utils::PointT<int>(point), fixedLayers(_fixedLayers) {}
        
    static void preorder(std::shared_ptr<SteinerTreeNode> node, std::function<void(std::shared_ptr<SteinerTreeNode>)> visit);
    static std::string getPythonString(std::shared_ptr<SteinerTreeNode> node);
};