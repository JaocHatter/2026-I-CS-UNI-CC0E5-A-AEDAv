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
    // redefiniendo algunos de los alias
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using MySelf = DoubleLinkedList<Trait>;
    using forward_iterator = DoubleLinkedListForwardIterator<MySelf>;
    friend forward_iterator;
    using backward_iterator = DoubleLinkedListBackwardIterator<MySelf>;
    friend backward_iterator;

    DoubleLinkedList() {}

    // TODO: Copy constructor
    DoubleLinkedList(const DoubleLinkedList &other) : m_pRoot(nullptr), m_tail(nullptr), m_size(0) {
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

    // Destructor Seguro
    virtual ~DoubleLinkedList() {
        std::unique_lock<std::shared_mutex> lock(this->m_mtx);
        
        Node* current = this->m_pRoot;
        while (current) {
            Node* next = current->getNext();
            delete current;
            current = next;             
        }
        
        this->m_pRoot = nullptr;
        this->m_tail = nullptr;
        this->m_size = 0;
    }

private:
    void internal_insert(Node* &curr, Node* &nPrev, const value_type &value, Ref ref) {
        if(!curr || m_comp(value, curr->getDataRef())){
            Node* tmp_node = new Node(value, ref, curr, nPrev);

            if (curr != nullptr) {
                curr->setPrev(newNode);
            } else {
                // Si no hay siguiente, el nuevo nodo es el nuevo tail
                this->m_tail = newNode;
            }
            curr = newNode;
            this->size++;
            return;
        }
        internal_insert(nPrev->getNextRef(), curr, value, ref);
    }
public:

    //un override porque esta 
    void insert(const value_type &value, Ref ref) override{
        unique_lock<shared_mutex> lock(this->m_mtx);
        internal_insert(this->m_pRoot, nullptr, value, ref);
        // el bucle while de linkedlist era innecesario, puesto que el tail ya se define en internal_insert!
        // esta podría ser una de las mejoras...
    }

    forward_iterator begin() { return forward_iterator(this, this->m_pRoot); }
    forward_iterator end()   { return forward_iterator(this, nullptr); }

    backward_iterator rbegin() { return backward_iterator(this,this->m_tail); }
    backward_iterator rend()   { return backward_iterator(this, nullptr); }

    
    // ForEach con control de concurrencia
    template <typename Func, typename... Args>
    void ForEach(Func func, Args &&... args) {
        unique_lock<shared_mutex> lock(this->m_mtx);
        ::ForEach(begin(), end(), func, forward<Args>(args)...);
    }

    // Para una DLL debería existir un ForEach backward
    template <typename Func,typename Args>
    void ReverseForEach(Func func, Args &&... args){
        unique_lock<shared_mutex> lock(this->m_mtx);
        ::ForEach(rbegin(), rend(), func, forward<Args>(args)...);
    }

    // override de push_back para dll
    void push_back(value_type value, Ref ref) override{
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node *newNode = new Node(value, ref, nullptr, this->m_tail);
        if (this->m_tail){
            this->m_tail->setNext(newNode);
        }else{
            this->m_pRoot = newNode;
        }
        this->m_tail = newNode;
        this->m_size++;
    }

    //operadores de stream output e input
    friend ostream& operator<<(ostream& os, const DoubleLinkedList& list) {
        shared_lock<shared_mutex>lock(list.m_mtx); 
        os << "[";
        Node* curr = list.m_pRoot;
        while(curr){
            os << "(" << curr->getData() << "," << act->getRef() << ")";
            if(curr->getNext()) {
                os << ",";
            } 
            curr = curr->getNext();
        }
        os << "]";
        return os;
    }

    friend istream& operator>>(istream& is, DoubleLinkedList& list) {
        char ch;
        if (!(is >> ch) || ch != '[') {
            is.clear(ios_base::failbit);
            return is;
        }
        value_type val;
        Ref ref;
        char comma, parenClose;
        while (is >> ch && ch != ']') {
            if (ch == '(') {
                if (is >> val >> comma >> ref >> parenClose) {
                    if (comma == ',' && parenClose == ')') {
                        list.push_back(val, ref);
                    }
                }
            }
        }
        return is;
    }

};