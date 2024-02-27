#include "GRTree.h"

void GRTreeNode::preorder(std::shared_ptr<GRTreeNode> node, std::function<void(std::shared_ptr<GRTreeNode>)> visit) {
    visit(node);
    for (auto& child : node->children) preorder(child, visit);
}

// void GRTreeNode::print(std::shared_ptr<GRTreeNode> node) {
//     preorder(node, [](std::shared_ptr<GRTreeNode> node) {
//         cout << *node << (node->children.size() > 0 ? " -> " : "");
//         for (auto& child : node->children) cout << *child << (child == node->children.back() ? "" : ", ");
//         cout << endl;
//     });
// }