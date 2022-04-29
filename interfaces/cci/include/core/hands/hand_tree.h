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

    template <typename T>
    class HandTree;

    class HandTreeIndex {
    public:
        template <typename T> friend class HandTree;

        HandTreeIndex(const HandTreeIndex &other);
        template <
            typename Iterator,
            typename Type = typename std::iterator_traits<Iterator>::value_type,
            typename = std::enable_if<std::is_arithmetic<Type>::value>
        >
        inline HandTreeIndex(Iterator begin, Iterator end);
        explicit HandTreeIndex(const std::vector<int> &traversalIndices);
        explicit HandTreeIndex(std::vector<int> &&traversalIndices);

        HandTreeIndex nth_child(int child) const;

    private:
        std::vector<int> traversalIndices;
    };

    template <typename T>
    class HandTreeNode {
    public:
        friend class HandTree<T>;

        HandTreeNode() = default;
        ~HandTreeNode() = default;

        HandTreeNode(HandTreeNode<T> &&other) : value(std::move(other.value)), children(std::move(other.children)) { }
        HandTreeNode(const HandTreeNode<T> &other) : value(other.value), children(other.children) { }

        explicit HandTreeNode(T value);

        const T &get_value() const;
        T &get_value();
        void set_value(T value);

    private:
        std::vector<HandTreeNode<T>> children;
        T value;

        void append_child(const HandTreeNode<T> &child);
        void append_child(HandTreeNode<T> &&child);

        const HandTreeNode<T> &get_nth_child(int n) const;
        HandTreeNode<T> &get_nth_child(int n);
    };

    template <typename T>
    class HandTree {
    public:
        HandTree();
        ~HandTree() = default;

        HandTree<T> get_subtree(HandTreeIndex &index);
        const HandTreeNode<T> &get_node(HandTreeIndex &index) const;
        HandTreeNode<T> &get_node(HandTreeIndex &index);
        template <typename Func>
        void for_each_node(Func &&func);

    private:
        HandTreeNode<T> root;

        HandTreeNode<T> create_thumb() const;
        HandTreeNode<T> create_finger() const;

        template <typename Func>
        static void for_each_node(HandTreeNode<T> &root, Func &&func);
    };

    template <typename T>
    HandTreeNode<T>::HandTreeNode(T value) : value(value) { }

    template <typename T>
    const T &HandTreeNode<T>::get_value() const {
        return value;
    }

    template <typename T>
    T &HandTreeNode<T>::get_value() {
        return const_cast<T&>(const_cast<const HandTreeNode<T>*>(this)->get_value());
    }

    template <typename T>
    void HandTreeNode<T>::set_value(T value) {
        this->value = value;
    }

    template <typename T>
    void HandTreeNode<T>::append_child(const HandTreeNode<T> &child) {
        children.push_back(child);
    }

    template <typename T>
    void HandTreeNode<T>::append_child(HandTreeNode<T> &&child) {
        children.push_back(std::move(child));
    }

    template <typename T>
    const HandTreeNode<T> &HandTreeNode<T>::get_nth_child(int n) const {
        return children[n];
    }

    // Reuse const version in non-const use-cases
    template <typename T>
    HandTreeNode<T> &HandTreeNode<T>::get_nth_child(int n) {
        return const_cast<HandTreeNode<T>&>(const_cast<const HandTreeNode<T>*>(this)->get_nth_child(n));
    }

    template<
        typename Iterator,
        typename Type,
        typename
    >
    inline HandTreeIndex::HandTreeIndex(Iterator begin, Iterator end) : traversalIndices(begin, end) { }

    template <typename T>
    HandTree<T>::HandTree() : root(HandTreeNode<T>()) {
        HandTreeNode<T> palm;

        palm.append_child(std::move(create_thumb()));
        palm.append_child(std::move(create_finger()));
        palm.append_child(std::move(create_finger()));
        palm.append_child(std::move(create_finger()));
        palm.append_child(std::move(create_finger()));

        root.append_child(std::move(palm));
    }

    template <typename T>
    HandTreeNode<T> HandTree<T>::create_thumb() const {
        HandTreeNode<T> distal;
        HandTreeNode<T> proximal;
        HandTreeNode<T> metacarpal;

        proximal.append_child(std::move(distal));
        metacarpal.append_child(std::move(proximal));

        return metacarpal;
    }

    template <typename T>
    HandTreeNode<T> HandTree<T>::create_finger() const {
        HandTreeNode<T> distal;
        HandTreeNode<T> middle;
        HandTreeNode<T> proximal;
        HandTreeNode<T> metacarpal;

        middle.append_child(std::move(distal));
        proximal.append_child(std::move(middle));
        metacarpal.append_child(std::move(proximal));

        return metacarpal;
    }

    template <typename T>
    HandTree<T> HandTree<T>::get_subtree(HandTreeIndex &index) {
        return HandTree<T>(get_node(index));
    }

    template <typename T>
    const HandTreeNode<T> &HandTree<T>::get_node(HandTreeIndex &index) const {
        const HandTreeNode<T> *result = &root;

        for (auto&& i = index.traversalIndices.begin(); i != index.traversalIndices.end(); ++i)
            result = &result->get_nth_child(*i);

        return *result;
    }

    template <typename T>
    HandTreeNode<T> &HandTree<T>::get_node(HandTreeIndex &index) {
        return const_cast<HandTreeNode<T>&>(const_cast<const HandTree<T>*>(this)->get_node(index));
    }

    template <typename T>
    template <typename Func>
    void HandTree<T>::for_each_node(Func &&func) {
        for_each_node(this->root, func);
    }

    template <typename T>
    template <typename Func>
    void HandTree<T>::for_each_node(HandTreeNode<T> &root, Func &&func) {
        func(root.get_value());

        for (auto it = root.children.begin(); it != root.children.end(); ++it) {
            for_each_node(*it, func);
        }
    }
}

namespace contactci::core::hands::constants {
    inline static HandTreeIndex FINGER_ROOT { std::vector<int>{0}};
    inline static HandTreeIndex THUMB_FINGER_METACARPAL { FINGER_ROOT.nth_child(0) };
    inline static HandTreeIndex THUMB_FINGER_PROXIMAL { THUMB_FINGER_METACARPAL.nth_child(0) };
    /* Thumb has no middle segment */
    inline static HandTreeIndex THUMB_FINGER_DISTAL { THUMB_FINGER_PROXIMAL.nth_child(0) };

    inline static HandTreeIndex INDEX_FINGER_METACARPAL { FINGER_ROOT.nth_child(1) };
    inline static HandTreeIndex INDEX_FINGER_PROXIMAL { INDEX_FINGER_METACARPAL.nth_child(0) };
    inline static HandTreeIndex INDEX_FINGER_MIDDLE { INDEX_FINGER_PROXIMAL.nth_child(0) };
    inline static HandTreeIndex INDEX_FINGER_DISTAL { INDEX_FINGER_MIDDLE.nth_child(0) };

    inline static HandTreeIndex MIDDLE_FINGER_METACARPAL { FINGER_ROOT.nth_child(2) };
    inline static HandTreeIndex MIDDLE_FINGER_PROXIMAL { MIDDLE_FINGER_METACARPAL.nth_child(0) };
    inline static HandTreeIndex MIDDLE_FINGER_MIDDLE { MIDDLE_FINGER_PROXIMAL.nth_child(0) };
    inline static HandTreeIndex MIDDLE_FINGER_DISTAL { MIDDLE_FINGER_MIDDLE.nth_child(0) };

    inline static HandTreeIndex RING_FINGER_METACARPAL { FINGER_ROOT.nth_child(3) };
    inline static HandTreeIndex RING_FINGER_PROXIMAL { RING_FINGER_METACARPAL.nth_child(0) };
    inline static HandTreeIndex RING_FINGER_MIDDLE { RING_FINGER_PROXIMAL.nth_child(0) };
    inline static HandTreeIndex RING_FINGER_DISTAL { RING_FINGER_MIDDLE.nth_child(0) };

    inline static HandTreeIndex LITTLE_FINGER_METACARPAL { FINGER_ROOT.nth_child(4) };
    inline static HandTreeIndex LITTLE_FINGER_PROXIMAL { LITTLE_FINGER_METACARPAL.nth_child(0) };
    inline static HandTreeIndex LITTLE_FINGER_MIDDLE { LITTLE_FINGER_PROXIMAL.nth_child(0) };
    inline static HandTreeIndex LITTLE_FINGER_DISTAL { LITTLE_FINGER_MIDDLE.nth_child(0) };
}

contactci::core::hands::HandTreeIndex::HandTreeIndex(const HandTreeIndex &other) : HandTreeIndex(other.traversalIndices) { }

contactci::core::hands::HandTreeIndex::HandTreeIndex(const std::vector<int> &traversalIndices)
        : HandTreeIndex(traversalIndices.begin(), traversalIndices.end()) { }

contactci::core::hands::HandTreeIndex::HandTreeIndex(std::vector<int> &&traversalIndices)
        : traversalIndices(std::move(traversalIndices)) { }

contactci::core::hands::HandTreeIndex contactci::core::hands::HandTreeIndex::nth_child(int child) const {
    std::vector<int> new_traversal(this->traversalIndices);

    new_traversal.push_back(child);

    return HandTreeIndex(new_traversal);
}
