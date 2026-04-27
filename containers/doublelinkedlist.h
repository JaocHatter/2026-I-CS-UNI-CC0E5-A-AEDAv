#pragma once
#include <iostream>
#include <cstddef> // size_t
#include <string>
#include <sstream>
#include <stdexcept>
#include <mutex>
#include <shared_mutex> 
#include <utility>
#include <tuple>
#include "util.h"
#include "../types.h"
#include "traits.h"
#include "linkedlist.h"
#include "general_iterator.h"

template <typename Container>
class DoubleLinkedListForwardIterator : public general_iterator<Container, LinkedListForwardIterator<Container>>{
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
class DoubleLinkedListBackwardIterator : public general_iterator<Container, LinkedListBackwardIterator<Container>>{
public:
    using MySelf = LinkedListBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;
    
    MySelf operator++() {
        if (this->m_pNode) {
            this->m_pNode = this->m_pNode->getPrev();
        }
        return *this;
    }
};

// TODO Los iteradores ahora son forward y backward
// Crear 2 nuevos i

// Acá estamos que heredamos de la clase LLNode, basicamente para heredar los metodos para avanzar de un nodo al siguiente
// Esta clase añade algunos metodos pero para retroceder a nodos previos
template <typename T>
class DLLNode : public LLNode<T, DLLNode<T>>{
    protected:
        // ya heredamos miembros como la data, la referencia y el siguiente nodo
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
public:
    using MySelf = DoubleLinkedList<Trait>;
    using forward_iterator = DoubleLinkedListForwardIterator<MySelf>;
    friend forward_iterator;
    using backward_iterator = DoubleLinkedListBackwardIterator<MySelf>;
    friend backward_iterator;

    DoubleLinkedList() {}

    // TODO: Copy constructor
    DoubleLinkedList(const DoubleLinkedList &other) : m_pRoot(nullptr), m_tail(nullptr), m_head(nullptr), m_size(0) {
        shared_lock<shared_mutex> lock(other.m_mtx);
        for(Node* curr = other.m_pRoot; curr != nullptr; curr = curr->getNext()) {
            push_back(curr->getData(), curr->getRef());
        }
    }

    //       Simplificar y abstraer el bucle de copia de Nodes
    //       Es posible que no necesites este constructor ya que lo heredaste

    // TODO: Move constructor
    DoubleLinkedList(LinkedList &&other) : m_pRoot(nullptr), m_tail(nullptr), m_size(0) {
        unique_lock<shared_mutex> lockOther(other.m_mtx);
        this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
        this->m_tail  = std::exchange(other.m_tail, nullptr);
        this->m_size  = std::exchange(other.m_size, 0);
    }

    // TODO: Copy assignment operator
    DoubleLinkedList& operator=(const DoubleLinkedList &other) {
        if(this != other){
            // limpiamos
            while (this->m_size > 0) pop_front(); 
            //copiamos
            shared_lock<shared_mutex> lock(other.m_mtx);
            for (Node* curr = other.m_pRoot; curr != nullptr; curr = curr->getNext()) {
                push_back(curr->getData(), curr->getRef());
            }
        }
        return *this;
    }

    // TODO: Move assignment operator
    DoubleLinkedList& operator=(DoubleLinkedList &&other) {
        if (this != &other) {
            while (this->m_size > 0) pop_front(); 
            unique_lock<shared_mutex> lockOther(other.m_mtx);
            this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
            this->m_tail  = std::exchange(other.m_tail, nullptr);
            this->m_size  = std::exchange(other.m_size, 0);
        }
        return *this;
    }
};