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
    size_t m_height;
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

protected:
    // Hook no bloqueante: el lock lo toma la base en insert()/operator>>.
    // Sobreescribir aqui (en vez de insert) mantiene una unica politica de
    // locking en la clase base y evita doble-lock del mutex no-recursivo.
    void internal_insert(Node*& pNode, value_type data) override {
        pNode = avl_insert(pNode, data);
    }

private:
    size_t node_height(Node* n) const { return n ? n->m_height : 0; }

    // con signo: la diferencia de alturas puede ser negativa (necesario para
    // que las comparaciones bf < -1 funcionen correctamente).
    long balance(Node* n) const {
        return n ? (long)node_height(n->m_pChild[0]) - (long)node_height(n->m_pChild[1]) : 0;
    }

    void update_height(Node* n) {
        if (n) n->m_height = 1 + max(node_height(n->m_pChild[0]), node_height(n->m_pChild[1]));
    }

    Node* rotate(Node* node, bool is_left) {
        Node* x_child  = node->m_pChild[is_left];
        Node* y_child = x_child->m_pChild[1 - is_left];
        x_child->m_pChild[1 - is_left] = node;
        node->m_pChild[is_left] = y_child;
        update_height(node);
        update_height(x_child);
        return x_child;
    }

    Node* avl_insert(Node* node, value_type data) {
        if (!node) { ++this->m_size; return new Node(data); }

        auto branch = !this->m_comp(node->m_data, data);
        node->m_pChild[branch] = avl_insert(node->m_pChild[branch], data);
        update_height(node);

        long bf = balance(node);

        // LL (data > pChild[0])  →  rotacion right
        if (bf > 1  &&  this->m_comp(node->m_pChild[0]->m_data, data))
            return rotate(node, false);
        // RR (data <= pChild[1]) →  rotacion left
        if (bf < -1 && !this->m_comp(node->m_pChild[1]->m_data, data))
            return rotate(node, true);
        // LR (data <= pChild[0]) →  rotacion left -> right
        if (bf > 1  && !this->m_comp(node->m_pChild[0]->m_data, data)) {
            node->m_pChild[0] = rotate(node->m_pChild[0], true);
            return rotate(node, false);
        }
        // RL (data > pChild[1])  →  rotacion right -> left
        if (bf < -1 &&  this->m_comp(node->m_pChild[1]->m_data, data)) {
            node->m_pChild[1] = rotate(node->m_pChild[1], false);
            return rotate(node, true);
        }
        return node;
    }
};

#endif // __BINARYTREE_AVL_H__
