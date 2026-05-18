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

template<typename Trait>
class BinaryTree{
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Compare    = typename Trait::Compare;
    private:
        Node    *m_pRoot;
        Compare  m_comp;
    public:
        BinaryTree() : m_pRoot(nullptr) {}
        ~BinaryTree() {}
        
        // No sirve ... muy anticuado y tiene muchos casos especiales
        void insert(value_type data){
            Node *newNode = new Node(data);
            if(m_pRoot == nullptr){
                m_pRoot = newNode;
            }else{
                Node *current = m_pRoot;
                while(true){
                    if(Compare()(data, current->data)){
                        if(current->left == nullptr){
                            current->left = newNode;
                            break;
                        }
                        current = current->left;
                    }else{
                        if(current->right == nullptr){
                            current->right = newNode;
                            break;
                        }
                        current = current->right;
                    }
                }
            }
        }
protected:
        void internal_insert(Node* &pNode, value_type data, Ref ref);

public: 
        void insert(value_type data, Ref ref){
            internal_insert(m_pRoot, data, ref);
        }
        void print(){
            print(m_pRoot);
        }
        void print(Node *node){
            if(node != nullptr){
                print(node->left);
                cout << node->data << " ";
                print(node->right);
            }
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