#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__
#include <iostream>
#include <cstddef>
#include <string>
#include <fstream>
#include <thread>
#include <stack>
#include <string>
#include <vector>
#include <sstream>
#include <mutex>
#include <shared_mutex>
#include <utility>
#include <algorithm>
#include "containers/general_iterator.h"
#include "containers/traits.h"
using namespace std;

// Definir la base CRTP 
// (necesaria para que `m_pChild` tenga 
// el tipo correcto al heredar en AVLNode)

template<typename Derived, typename T>
struct BinaryTreeNodeBase {
    using value_type = T;
    T        m_data;
    Derived* m_pChild[2];
    BinaryTreeNodeBase(T data) : m_data(data), m_pChild{nullptr, nullptr} {}
    virtual ~BinaryTreeNodeBase() = default;
    T& getDataRef()       { return m_data; }
    T  getData()    const { return m_data; }
};

template<typename T>
struct BinaryTreeNode : BinaryTreeNodeBase<BinaryTreeNode<T>, T> {
    BinaryTreeNode(T data) : BinaryTreeNodeBase<BinaryTreeNode<T>, T>(data) {}
};

// Utilizar:
//    AscendingTrait<BinaryTreeNode<T>> o 
//    DescendingTrait<BinaryTreeNode<T>>

template<typename T> using AscendingBSTrait  = AscendingTrait<BinaryTreeNode<T>>;
template<typename T> using DescendingBSTrait = DescendingTrait<BinaryTreeNode<T>>;


// funciones anticuadas eliminadas
template<typename Trait>
class BinaryTree{
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Comp    = typename Trait::Comp;
    using MySelf     = BinaryTree<Trait>;
    protected:
        Node    *m_pRoot;
        Comp  m_comp;
        size_t m_size;
        mutable shared_mutex m_mtx;
    public:
        BinaryTree() : m_pRoot(nullptr) m_size(0) {}
        ~BinaryTree() { internal_destroy(m_pRoot); }
    private:
        void internal_destroy(Node* node) {
            if (!node) return;
            internal_destroy(node->m_pChild[0]);
            internal_destroy(node->m_pChild[1]);
            delete node;
        }
};

// Codigo hecho en clase
template<typename Trait>
void BinaryTree<Trait>::internal_insert(Node* &pNode, value_type data, Ref ref){
    if( pNode == nullptr ){
        pNode = new Node(data);
        return;
    }
    auto branch = !m_comp(pNode->m_data, data);
    internal_insert(pNode->m_pChild[branch], data, ref);
}
#endif // __BINARYTREE_H__ 