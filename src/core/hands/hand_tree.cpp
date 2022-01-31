//
// Created by john_contactci on 1/31/2022.
//

#include "../../include/core/hands/hand_tree.h"

using namespace contactci::core::hands;

HandTreeNode::HandTreeNode() = default;
HandTreeNode::~HandTreeNode() = default;

HandTreeIndex::HandTreeIndex(const HandTreeIndex &other) : HandTreeIndex(other.traversalIndices) { }

HandTreeIndex::HandTreeIndex(const std::vector<int> &traversalIndices) : HandTreeIndex(traversalIndices.begin(), traversalIndices.end()) {
}

HandTreeIndex HandTreeIndex::nth_child(int child) {
    std::vector<int> new_traversal;

    std::copy(this->traversalIndices.begin(), this->traversalIndices.end(), new_traversal.begin());

    new_traversal.push_back(child);

    return HandTreeIndex(new_traversal);
}

HandTree::HandTree() : root(HandTreeNode()) { }
HandTree::HandTree(const HandTreeNode& root) : root(root) { }

HandTree::~HandTree() = default;

HandTree HandTree::get_subtree(HandTreeIndex &index) {
    return HandTree(get_node(index));
}

HandTreeNode HandTree::get_node(HandTreeIndex &index) {
    // TODO
    return this->root;
}