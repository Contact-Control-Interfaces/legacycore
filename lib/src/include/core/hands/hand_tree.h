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
    //     │       └── thumb distal
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

    template <typename T>
    class HandTree;

    class HandTreeIndex {
    public:
        template <typename T> friend class HandTree;
        static HandTreeIndexConstants CONSTANTS;

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
        friend class HandTree<T>;

        HandTreeNode() = default;
        ~HandTreeNode() = default;

        explicit HandTreeNode(T value);

        T get_value() const;
        void set_value(T value);

    private:
        std::vector<HandTreeNode<T>> children;
        T value;

        void append_child(const HandTreeNode<T> &child);
        HandTreeNode<T> get_nth_child(int n) const;
    };

    // TODO build tree
    template <typename T>
    class HandTree {
    public:
        HandTree();
        ~HandTree() = default;

        HandTree<T> get_subtree(HandTreeIndex &index);
        HandTreeNode<T> get_node(HandTreeIndex &index);

    private:
        HandTreeNode<T> root;

        HandTreeNode<T> create_thumb() const;
        HandTreeNode<T> create_finger() const;
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

    template <typename T>
    void HandTreeNode<T>::append_child(const HandTreeNode<T> &child) {
        children.push_back(child);
    }

    //TODO return ref?
    template <typename T>
    HandTreeNode<T> HandTreeNode<T>::get_nth_child(int n) const {
        return children[n];
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
    HandTree<T>::HandTree() : root(HandTreeNode<T>()) {
        HandTreeNode<T> palm;

        palm.append_child(create_thumb());
        palm.append_child(create_finger());
        palm.append_child(create_finger());
        palm.append_child(create_finger());
        palm.append_child(create_finger());

        root.append_child(palm);
    }

    template <typename T>
    HandTreeNode<T> HandTree<T>::create_thumb() const {
        HandTreeNode<T> distal;
        HandTreeNode<T> proximal;
        HandTreeNode<T> metacarpal;

        proximal.append_child(distal);
        metacarpal.append_child(proximal);

        return metacarpal;
    }

    template <typename T>
    HandTreeNode<T> HandTree<T>::create_finger() const {
        HandTreeNode<T> distal;
        HandTreeNode<T> middle;
        HandTreeNode<T> proximal;
        HandTreeNode<T> metacarpal;

        middle.append_child(distal);
        proximal.append_child(middle);
        metacarpal.append_child(proximal);

        return metacarpal;
    }

    template <typename T>
    HandTree<T> HandTree<T>::get_subtree(HandTreeIndex &index) {
        return HandTree<T>(get_node(index));
    }

    template <typename T>
    HandTreeNode<T> HandTree<T>::get_node(HandTreeIndex &index) {
        HandTreeNode<T> result = root;

        for (auto&& i = index.traversalIndices.begin(); i != index.traversalIndices.end(); ++i)
            result = root.get_nth_child(i);

        return result;
    }
}
