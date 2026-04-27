#pragma once
#include "linkedlist.h"
#include "general_iterator.h"

template <typename Container>
class LinkedListForwardIterator : public general_iterator<Container, LinkedListForwardIterator<Container>>{
public:
    using MySelf = LinkedListForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;
    
    MySelf operator++() {
        if (this->m_pNode) {
            this->m_pNode = this->m_pNode->getNext();
        }
        return *this;
    }
};

template <typename Container>
class LinkedListBackwardIterator : public general_iterator<Container, LinkedListBackwardIterator<Container>>{
public:
    using MySelf = LinkedListBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;
    
    MySelf operator++() {
        if (this->m_pNode) {
            this->m_pNode = this->m_pNode->getPrevious();
        }
        return *this;
    }
};

// TODO Los iteradores ahora son forward y backward
// Crear 2 nuevos i
template <typename T>
class DLLNode : public LLNode<T, DLLNode<T>>{
    private:
        Node *m_pPrev;
    public:
        DLLNode() : LLNode<T, DLLNode<T>>(), m_pPrev(nullptr) {}
        DLLNode(T data, Ref ref, Node *next = nullptr, Node *prev = nullptr) : LLNode<T, DLLNode<T>>(data, ref, next), m_pPrev(prev) {}

        Node*  getPrev() const     { return m_pPrev; }
        void   setPrev(Node *prev) { m_pPrev = prev; }
        Node*& getPrevRef()        { return m_pPrev; }

};

template <typename T>
struct AscendingDLLTrait : BaseTrait<T, less<T>>{
    using Node = DLLNode<T>;
};

template <typename T>
struct DescendingDLLTrait : BaseTrait<T, greater<T>>{
    using Node = DLLNode<T>;
};

template <typename Trait>
class DoubleLinkedList : public LinkedList<Trait>{

    // TODO: Copy constructor
    //       Simplificar y abstraer el bucle de copia de Nodes
    //       Es posible que no necesites este constructor ya que lo heredaste

    // TODO: Move constructor
    // TODO: Copy assignment operator
    // TODO: Move assignment operator
};