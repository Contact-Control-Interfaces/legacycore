//
// Created by john_contactci on 1/31/2022.
//

#include "core/hands/hand_tree.h"

using namespace contactci::core::hands;

HandTreeIndex::HandTreeIndex(const HandTreeIndex &other) : HandTreeIndex(other.traversalIndices) { }

HandTreeIndex::HandTreeIndex(const std::vector<int> &traversalIndices)
    : HandTreeIndex(traversalIndices.begin(), traversalIndices.end()) { }

HandTreeIndex HandTreeIndex::nth_child(int child) const {
    std::vector<int> new_traversal;

    std::copy(this->traversalIndices.begin(), this->traversalIndices.end(), new_traversal.begin());

    new_traversal.push_back(child);

    return HandTreeIndex(new_traversal);
}

HandTreeIndexConstants::HandTreeIndexConstants()
    : FINGER_ROOT(std::vector<int>{0, 0}),
      THUMB_FINGER_METACARPAL(FINGER_ROOT.nth_child(0)),
      THUMB_FINGER_PROXIMAL(THUMB_FINGER_METACARPAL.nth_child(0)),
        /* Thumb has no middle segment */
      THUMB_FINGER_DISTAL(THUMB_FINGER_PROXIMAL.nth_child(0)),

      INDEX_FINGER_METACARPAL(FINGER_ROOT.nth_child(1)),
      INDEX_FINGER_PROXIMAL(INDEX_FINGER_METACARPAL.nth_child(0)),
      INDEX_FINGER_MIDDLE(INDEX_FINGER_PROXIMAL.nth_child(0)),
      INDEX_FINGER_DISTAL(INDEX_FINGER_MIDDLE.nth_child(0)),

      MIDDLE_FINGER_METACARPAL(FINGER_ROOT.nth_child(2)),
      MIDDLE_FINGER_PROXIMAL(MIDDLE_FINGER_METACARPAL.nth_child(0)),
      MIDDLE_FINGER_MIDDLE(MIDDLE_FINGER_PROXIMAL.nth_child(0)),
      MIDDLE_FINGER_DISTAL(MIDDLE_FINGER_MIDDLE.nth_child(0)),

      RING_FINGER_METACARPAL(FINGER_ROOT.nth_child(3)),
      RING_FINGER_PROXIMAL(RING_FINGER_METACARPAL.nth_child(0)),
      RING_FINGER_MIDDLE(RING_FINGER_PROXIMAL.nth_child(0)),
      RING_FINGER_DISTAL(RING_FINGER_MIDDLE.nth_child(0)),

      LITTLE_FINGER_METACARPAL(FINGER_ROOT.nth_child(4)),
      LITTLE_FINGER_PROXIMAL(LITTLE_FINGER_METACARPAL.nth_child(0)),
      LITTLE_FINGER_MIDDLE(LITTLE_FINGER_PROXIMAL.nth_child(0)),
      LITTLE_FINGER_DISTAL(LITTLE_FINGER_MIDDLE.nth_child(0))
{ /* This is intentional */ }