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
class DoubleLinkedListBackwardIterator : public general_iterator<Container, DoubleLinkedListBackwardIterator<Container>>{
public:
    using MySelf = DoubleLinkedListBackwardIterator<Container>;
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
class DLLNode : public LLNodeBase<T, DLLNode<T>>{
    protected:
        using Node = DLLNode<T>;
        Node *m_pPrev;
    public:
        DLLNode() : LLNodeBase<T, DLLNode<T>>(), m_pPrev(nullptr) {}
        DLLNode(T data, Ref ref, Node *next = nullptr, Node *prev = nullptr) : LLNodeBase<T, DLLNode<T>>(data, ref, next), m_pPrev(prev) {}

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
    DoubleLinkedList(const DoubleLinkedList &other) {
        shared_lock<shared_mutex> lock(other.m_mtx);
        for(Node* curr = other.m_pRoot; curr != nullptr; curr = curr->getNext()) {
            push_back(curr->getData(), curr->getRef());
        }
    }

    //       Simplificar y abstraer el bucle de copia de Nodes
    //       Es posible que no necesites este constructor ya que lo heredaste

    // TODO: Move constructor
    DoubleLinkedList(DoubleLinkedList &&other) {
        unique_lock<shared_mutex> lockOther(other.m_mtx);
        this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
        this->m_tail  = std::exchange(other.m_tail, nullptr);
        this->m_size  = std::exchange(other.m_size, 0);
    }

    // TODO: Copy assignment operator
    DoubleLinkedList& operator=(const DoubleLinkedList &other) {
        if(this != &other){
            // limpiamos
            while (this->m_size > 0) this->pop_front();
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
            while (this->m_size > 0) this->pop_front();
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
    void internal_insert(Node* &curr, Node* prev, const value_type &value, Ref ref) {
        if (!curr || this->m_comp(value, curr->getDataRef())) {
            Node* new_node = new Node(value, ref, curr, prev);
            if (curr != nullptr)
                curr->setPrev(new_node);
            else
                this->m_tail = new_node;
            curr = new_node;
            this->m_size++;
            return;
        }
        internal_insert(curr->getNextRef(), curr, value, ref);
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
    template <typename Func, typename... Args>
    void ReverseForEach(Func func, Args &&... args){
        unique_lock<shared_mutex> lock(this->m_mtx);
        ::ForEach(rbegin(), rend(), func, forward<Args>(args)...);
    }

    // push_front como mejor #1
    void push_front(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* new_node = new Node(value, ref, this->m_pRoot, nullptr);
        if (this->m_pRoot)
            this->m_pRoot->setPrev(new_node);
        else
            this->m_tail = new_node;
        this->m_pRoot = new_node;
        this->m_size++;
    }

    // pop_front como mejora #2
    std::tuple<value_type, Ref> pop_front() override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        if (!this->m_pRoot) throw runtime_error("La lista esta vacia");
        Node* temp = this->m_pRoot;
        auto result = std::make_tuple(temp->getData(), temp->getRef());
        if (this->m_size == 1) {
            this->m_pRoot = this->m_tail = nullptr;
        } else {
            this->m_pRoot = temp->getNext();
            this->m_pRoot->setPrev(nullptr);
        }
        delete temp;
        this->m_size--;
        return result;
    }

    void push_back(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* new_node = new Node(value, ref, nullptr, this->m_tail);
        if (this->m_tail)
            this->m_tail->setNext(new_node);
        else
            this->m_pRoot = new_node;
        this->m_tail = new_node;
        this->m_size++;
    }

    //operadores de stream output e input
    friend ostream& operator<<(ostream& os, const DoubleLinkedList& list) {
        shared_lock<shared_mutex>lock(list.m_mtx); 
        os << "[";
        Node* curr = list.m_pRoot;
        while(curr){
            os << "(" << curr->getData() << "," << curr->getRef() << ")";
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