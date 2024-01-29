#include <iostream>
#include "vector"

using namespace std;

class GRPoint {
public:
    int x;
    int y;
    int layerIdx;

    GRPoint(int l, int _x, int _y): layerIdx(l), x(_x), y(_y) {}

    friend inline std::ostream& operator<<(std::ostream& os, const GRPoint& pt) {
        os << "(" << pt.layerIdx << ", " << pt.x << ", " << pt.y << ")";
        return os;
    }
};

class GRTreeNode: public GRPoint {
public:
    vector<GRTreeNode*> children;

    GRTreeNode(int l, int _x, int _y): GRPoint(l, _x, _y) {}
    GRTreeNode(const GRPoint& point): GRPoint(point) {}

    void preorder(void (*visit)(GRTreeNode*));
    void print();
};