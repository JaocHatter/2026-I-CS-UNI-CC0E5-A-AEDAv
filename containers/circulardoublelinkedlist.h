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
#include "circularlinkedlist.h"

template <typename Container>
class CDLLForwardIterator : public general_iterator<Container, CDLLForwardIterator<Container>> {
public:
    using MySelf = CDLLForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;

    MySelf& operator++() {
        if (this->m_pNode) {
            auto* next = this->m_pNode->getNext();
            // si el siguiente nodo es la raiz, significa que has dado una vuelta al anillo,
            // y por tanto te retornará nullptr... esto nos ayuda a evitar bucles infinitos
            this->m_pNode = (next == this->m_pContainer->m_pRoot) ? nullptr : next;
        }
        return *this;
    }
};

template <typename Container>
class CDLLBackwardIterator : public general_iterator<Container, CDLLBackwardIterator<Container>> {
public:
    using MySelf = CDLLBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;

    MySelf& operator++() {
        if (this->m_pNode) {
            auto* prev = this->m_pNode->getPrev();
            this->m_pNode = (prev == this->m_pContainer->m_tail) ? nullptr : prev;
        }
        return *this;
    }
};

template <typename T>
struct AscendingCDLLTrait : BaseTrait<T, less<T>> {
    using Node = DLLNode<T>;
};

template <typename T>
struct DescendingCDLLTrait : BaseTrait<T, greater<T>> {
    using Node = DLLNode<T>;
};

// heredamos de CircleLinkedList, se agregó punteros prev para doble enlace
template <typename Trait>
class CircularDoubleLinkedList : public CircleLinkedList<Trait> {
public:
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using MySelf     = CircularDoubleLinkedList<Trait>;
    using forward_iterator  = CDLLForwardIterator<MySelf>;
    friend forward_iterator;
    using backward_iterator = CDLLBackwardIterator<MySelf>;
    friend backward_iterator;

    CircularDoubleLinkedList() {}

    CircularDoubleLinkedList(const CircularDoubleLinkedList& other) {
        shared_lock<shared_mutex> lock(other.m_mtx);
        if (!other.m_pRoot) return;
        Node* curr = other.m_pRoot;
        do {
            internal_push_back(curr->getData(), curr->getRef());
            curr = curr->getNext();
        } while (curr != other.m_pRoot);
    }

    CircularDoubleLinkedList(CircularDoubleLinkedList&& other) {
        unique_lock<shared_mutex> lock(other.m_mtx);
        this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
        this->m_tail  = std::exchange(other.m_tail,  nullptr);
        this->m_size  = std::exchange(other.m_size,  0);
    }

    // Done: operador= de copia
    CircularDoubleLinkedList& operator=(const CircularDoubleLinkedList& other) {
        if (this != &other) {
            unique_lock<shared_mutex> lockThis(this->m_mtx);
            clear_unlocked();
            shared_lock<shared_mutex> lockOther(other.m_mtx);
            if (other.m_pRoot) {
                Node* curr = other.m_pRoot;
                do {
                    internal_push_back(curr->getData(), curr->getRef());
                    curr = curr->getNext();
                } while (curr != other.m_pRoot);
            }
        }
        return *this;
    }

    // Done: operador= move
    CircularDoubleLinkedList& operator=(CircularDoubleLinkedList&& other) {
        if (this != &other) {
            unique_lock<shared_mutex> lockThis(this->m_mtx);
            clear_unlocked();
            unique_lock<shared_mutex> lockOther(other.m_mtx);
            this->m_pRoot = std::exchange(other.m_pRoot, nullptr);
            this->m_tail  = std::exchange(other.m_tail,  nullptr);
            this->m_size  = std::exchange(other.m_size,  0);
        }
        return *this;
    }

    // El destructor de CircularLinkedList es valido, entonces no es necesario crear una destrucción segura nuevamente

private:
    // función auxiliar que me ayuda a eliminar cada uno de los nodos
    void clear_unlocked() {
        if (this->m_tail) this->m_tail->setNext(nullptr);
        Node* curr = this->m_pRoot;
        while (curr) {
            Node* next = curr->getNext();
            delete curr;
            curr = next;
        }
        this->m_pRoot = this->m_tail = nullptr;
        this->m_size = 0;
    }

    void internal_push_back(value_type value, Ref ref) {
        Node* new_node = new Node(value, ref);
        if (!this->m_pRoot) {
            new_node->setNext(new_node);
            new_node->setPrev(new_node);
            this->m_pRoot = this->m_tail = new_node;
        } else {
            new_node->setNext(this->m_pRoot);
            new_node->setPrev(this->m_tail);
            this->m_tail->setNext(new_node);
            this->m_pRoot->setPrev(new_node);
            this->m_tail = new_node;
        }
        this->m_size++;
    }

    void internal_push_front(value_type value, Ref ref) {
        Node* new_node = new Node(value, ref);
        if (!this->m_pRoot) {
            new_node->setNext(new_node);
            new_node->setPrev(new_node);
            this->m_pRoot = this->m_tail = new_node;
        } else {
            new_node->setNext(this->m_pRoot);
            new_node->setPrev(this->m_tail);
            this->m_tail->setNext(new_node);
            this->m_pRoot->setPrev(new_node);
            this->m_pRoot = new_node;
        }
        this->m_size++;
    }

    // Insercionn
    void internal_insert(const value_type& value, Ref ref) {
        if (!this->m_pRoot || this->m_comp(value, this->m_pRoot->getDataRef())) {
            internal_push_front(value, ref);
            return;
        }
        Node* curr = this->m_pRoot;
        while (curr->getNext() != this->m_pRoot &&
               !this->m_comp(value, curr->getNext()->getDataRef())) {
            curr = curr->getNext();
        }
        // Insert after curr
        Node* new_node = new Node(value, ref);
        new_node->setNext(curr->getNext());
        new_node->setPrev(curr);
        curr->getNext()->setPrev(new_node);
        curr->setNext(new_node);
        if (curr == this->m_tail)
            this->m_tail = new_node;
        this->m_size++;
    }

public:
    void push_back(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        internal_push_back(value, ref);
    }

    void push_front(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        internal_push_front(value, ref);
    }

    std::tuple<value_type, Ref> pop_front() override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        if (!this->m_pRoot) throw runtime_error("lista vacia!");
        Node* temp = this->m_pRoot;
        auto result = std::make_tuple(temp->getData(), temp->getRef());
        if (this->m_size == 1) {
            this->m_pRoot = this->m_tail = nullptr;
        } else {
            this->m_pRoot = temp->getNext();
            this->m_pRoot->setPrev(this->m_tail);
            this->m_tail->setNext(this->m_pRoot);
        }
        delete temp;
        this->m_size--;
        return result;
    }

    std::tuple<value_type, Ref> pop_back() {
        unique_lock<shared_mutex> lock(this->m_mtx);
        if (!this->m_pRoot) throw runtime_error("La lista esta vacia");
        Node* temp = this->m_tail;
        auto result = std::make_tuple(temp->getData(), temp->getRef());
        if (this->m_size == 1) {
            this->m_pRoot = this->m_tail = nullptr;
        } else {
            this->m_tail = temp->getPrev();
            this->m_tail->setNext(this->m_pRoot);
            this->m_pRoot->setPrev(this->m_tail);
        }
        delete temp;
        this->m_size--;
        return result;
    }

    void insert(const value_type& value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        internal_insert(value, ref);
    }

    forward_iterator  begin()  { return forward_iterator(this, this->m_pRoot); }
    forward_iterator  end()    { return forward_iterator(this, nullptr); }
    backward_iterator rbegin() { return backward_iterator(this, this->m_tail); }
    backward_iterator rend()   { return backward_iterator(this, nullptr); }

    // Done: ForEach con Control concurrente
    template <typename Func, typename... Args>
    void ForEach(Func func, Args&&... args) {
        unique_lock<shared_mutex> lock(this->m_mtx);
        ::ForEach(begin(), end(), func, forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    void ReverseForEach(Func func, Args&&... args) {
        unique_lock<shared_mutex> lock(this->m_mtx);
        ::ForEach(rbegin(), rend(), func, forward<Args>(args)...);
    }

    friend ostream& operator<<(ostream& os, const CircularDoubleLinkedList& list) {
        shared_lock<shared_mutex> lock(list.m_mtx);
        os << "[";
        if (list.m_pRoot) {
            Node* curr = list.m_pRoot;
            do {
                os << "(" << curr->getData() << "," << curr->getRef() << ")";
                curr = curr->getNext();
                if (curr != list.m_pRoot) os << ",";
            } while (curr != list.m_pRoot);
        }
        os << "]";
        return os;
    }

    friend istream& operator>>(istream& is, CircularDoubleLinkedList& list) {
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
