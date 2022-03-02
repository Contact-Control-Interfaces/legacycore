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

    template <typename T>
    class HandTreeNode {
    public:
        explicit HandTreeNode(T value);
        ~HandTreeNode() = default;

        T get_value() const;
        void set_value(T value);
    private:
        T value;
    };

    template <typename T>
    class HandTree {
    private:
        HandTreeNode<T> root;

    public:
        explicit HandTree(const HandTreeNode<T>& root);
        ~HandTree() = default;

        HandTree<T> get_subtree(HandTreeIndex &index);
        HandTreeNode<T> get_node(HandTreeIndex &index);
    };

    template <typename T>
    HandTreeNode<T>::HandTreeNode(T value) : value(value) { }

    template <typename T>
    T HandTreeNode<T>::get_value() const {
        return value;
    }

    template <typename T>
    void HandTreeNode<T>::set_value(T value) {
        this->value = value;
    }

    template<
        typename Iterator,
        typename Type,
        typename
    >
    inline HandTreeIndex::HandTreeIndex(Iterator begin, Iterator end) {
        std::copy(begin, end, this->traversalIndices.begin());
    }

    template <typename T>
    HandTree<T>::HandTree(const HandTreeNode<T>& root) : root(root) { }

    template <typename T>
    HandTree<T> HandTree<T>::get_subtree(HandTreeIndex &index) {
        return HandTree<T>(get_node(index));
    }

    template <typename T>
    HandTreeNode<T> HandTree<T>::get_node(HandTreeIndex &index) {
        // TODO
        return this->root;
    }
}






