#ifndef __BINARYTREE_AVL_H__
#define __BINARYTREE_AVL_H__

#include <algorithm>
#include <shared_mutex>
#include <utility>
#include "BinaryTree.h"
using namespace std;

// heredamos de la clase nodo base
template<typename T>
struct AVLNode : BinaryTreeNodeBase<AVLNode<T>, T> {
    int m_height;
    AVLNode(T data) : BinaryTreeNodeBase<AVLNode<T>, T>(data), m_height(1) {}
};

template<typename T> using AscendingAVLTrait  = AscendingTrait<AVLNode<T>>;
template<typename T> using DescendingAVLTrait = DescendingTrait<AVLNode<T>>;


template<typename Trait>
class BinaryTreeAVL : public BinaryTree<Trait> {
    using Base       = BinaryTree<Trait>;
    using Node       = typename Base::Node;
    using value_type = typename Base::value_type;

public:
    using Base::Base;

    void insert(value_type data) override {
        unique_lock lock(this->m_mtx);
        this->m_pRoot = avl_insert(this->m_pRoot, data);
    }

private:
    int node_height(Node* n) const { return n ? n->m_height : 0; }

    int balance(Node* n) const {
        return n ? node_height(n->m_pChild[0]) - node_height(n->m_pChild[1]) : 0;
    }

    void update_height(Node* n) {
        if (n) n->m_height = 1 + max(node_height(n->m_pChild[0]), node_height(n->m_pChild[1]));
    }

    Node* rotate_right(Node* y) {
        Node* x  = y->m_pChild[0];
        Node* T2 = x->m_pChild[1];
        x->m_pChild[1] = y;
        y->m_pChild[0] = T2;
        update_height(y);
        update_height(x);
        return x;
    }

    Node* rotate_left(Node* x) {
        Node* y  = x->m_pChild[1];
        Node* T2 = y->m_pChild[0];
        y->m_pChild[0] = x;
        x->m_pChild[1] = T2;
        update_height(x);
        update_height(y);
        return y;
    }

    Node* avl_insert(Node* node, value_type data) {
        if (!node) { ++this->m_size; return new Node(data); }

        auto branch = !this->m_comp(node->m_data, data);
        node->m_pChild[branch] = avl_insert(node->m_pChild[branch], data);
        update_height(node);

        int bf = balance(node);

        // LL (data > pChild[0])  →  rotacion right 
        if (bf > 1  &&  this->m_comp(node->m_pChild[0]->m_data, data))
            return rotate_right(node);
        // RR (data <= pChild[1]) →  rotacion left
        if (bf < -1 && !this->m_comp(node->m_pChild[1]->m_data, data))
            return rotate_left(node);
        // LR (data <= pChild[0]) →  rotacion left -> right 
        if (bf > 1  && !this->m_comp(node->m_pChild[0]->m_data, data)) {
            node->m_pChild[0] = rotate_left(node->m_pChild[0]);
            return rotate_right(node);
        }
        // RL (data > pChild[1])  →  rotacion right -> left
        if (bf < -1 &&  this->m_comp(node->m_pChild[1]->m_data, data)) {
            node->m_pChild[1] = rotate_right(node->m_pChild[1]);
            return rotate_left(node);
        }
        return node;
    }
};

#endif // __BINARYTREE_AVL_H__
