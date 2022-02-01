//
// Created by john_contactci on 1/31/2022.
//

#pragma once

#include <iterator>
#include <vector>

namespace contactci::core::hands {

    // wrist
    // └── palm
    //     ├── thumb metacarpal
    //     │   └── thumb proximal
    //     │       └── thumb middle
    //     │           └── thumb distal
    //     ├── index metacarpal
    //     │   └── index proximal
    //     │       └── index middle
    //     │           └── index distal
    //     ├── middle metacarpal
    //     │   └── middle proximal
    //     │       └── middle middle
    //     │           └── middle distal
    //     ├── ring metacarpal
    //     │   └── ring proximal
    //     │       └── ring middle
    //     │           └── ring distal
    //     └── little metacarpal
    //         └── little proximal
    //             └── little middle
    //                 └── little distal

    class HandTreeIndexConstants;

    class HandTreeIndex {
    private:
        std::vector<int> traversalIndices;

        HandTreeIndex(const HandTreeIndex &other);

        template <
            typename Iterator,
            typename Type = typename std::iterator_traits<Iterator>::value_type,
            typename = std::enable_if<std::is_arithmetic<Type>::value>
        >
        inline HandTreeIndex(Iterator begin, Iterator end);
        explicit HandTreeIndex(const std::vector<int> &traversalIndices);
        HandTreeIndex nth_child(int child) const;

        friend class HandTreeIndexConstants;

    public:
        static HandTreeIndexConstants CONSTANTS;
    };

    class HandTreeIndexConstants {
    public:
        HandTreeIndexConstants();

        const HandTreeIndex FINGER_ROOT;

        const HandTreeIndex THUMB_FINGER_METACARPAL;
        const HandTreeIndex THUMB_FINGER_PROXIMAL;
        /* Thumb has no middle segment */
        const HandTreeIndex THUMB_FINGER_DISTAL;

        const HandTreeIndex INDEX_FINGER_METACARPAL;
        const HandTreeIndex INDEX_FINGER_PROXIMAL;
        const HandTreeIndex INDEX_FINGER_MIDDLE;
        const HandTreeIndex INDEX_FINGER_DISTAL;

        const HandTreeIndex MIDDLE_FINGER_METACARPAL;
        const HandTreeIndex MIDDLE_FINGER_PROXIMAL;
        const HandTreeIndex MIDDLE_FINGER_MIDDLE;
        const HandTreeIndex MIDDLE_FINGER_DISTAL;

        const HandTreeIndex RING_FINGER_METACARPAL;
        const HandTreeIndex RING_FINGER_PROXIMAL;
        const HandTreeIndex RING_FINGER_MIDDLE;
        const HandTreeIndex RING_FINGER_DISTAL;

        const HandTreeIndex LITTLE_FINGER_METACARPAL;
        const HandTreeIndex LITTLE_FINGER_PROXIMAL;
        const HandTreeIndex LITTLE_FINGER_MIDDLE;
        const HandTreeIndex LITTLE_FINGER_DISTAL;
    };

    class HandTreeNode {
    public:
        HandTreeNode();
        ~HandTreeNode();
    };

    class HandTree {
    private:
        HandTreeNode root;

    public:
        HandTree();
        explicit HandTree(const HandTreeNode& root);
        ~HandTree();

        HandTree get_subtree(HandTreeIndex &index);
        HandTreeNode get_node(HandTreeIndex &index);
    };
}

template<
        typename Iterator,
        typename Type,
        typename
>
inline contactci::core::hands::HandTreeIndex::HandTreeIndex(Iterator begin, Iterator end) {
    std::copy(begin, end, this->traversalIndices.begin());
}