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
#include "general_iterator.h"
#include "util.h"
#include "../types.h"
#include "traits.h"
using namespace std;

// Iterador Forward
template <typename Container>
class CircularLinkedListForwardIterator : public general_iterator<Container, CircularLinkedListForwardIterator<Container>>{
public:
    using MySelf = CircularLinkedListForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;

    MySelf& operator++() {
        if (this->m_pNode) {
            this->m_pNode = this->m_pNode->getNext();
        }
        return *this;
    }
};

// Nodo de Lista Circular
template <typename T>
class CLLNode {
public:
    using Node = CLLNode<T>;
private:
    T    m_data;
    Ref  m_ref;
    Node *m_next;
public:
    CLLNode() : m_data(T()), m_ref(Ref()), m_next(nullptr) {}
    CLLNode(T data, Ref ref) : m_data(data), m_ref(ref), m_next(nullptr) {}
    CLLNode(T data, Ref ref, Node *next) : m_data(data), m_ref(ref), m_next(next) {}
    virtual ~CLLNode() {}

    T      getData() const     { return m_data; }
    T&     getDataRef()        { return m_data; }
    void   setData(T data)     { m_data = data; }
    Ref    getRef() const      { return m_ref; }
    void   setRef(Ref ref)     { m_ref = ref; }
    Node*  getNext() const     { return m_next; }
    Node*& getNextRef()        { return m_next; }
    void   setNext(Node *next) { m_next = next; }
};

// Traits de Ordenamiento
template <typename T>
struct AscendingCircleLinkedListTrait {
    using value_type = T;
    using Node = CLLNode<T>;
    using Comp = less<T>;
};

template <typename T>
struct DescendingCircleLinkedListTrait {
    using value_type = T;
    using Node = CLLNode<T>;
    using Comp = greater<T>;
};

// Contenedor Principal CircleLinkedList
// m_tail->next == m_pRoot SIEMPRE (salvo q la lista esté vacía)
template <typename Trait>
class CircleLinkedList {
public:
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Comp       = typename Trait::Comp;
    using MySelf     = CircleLinkedList<Trait>;

    using forward_iterator = CircularLinkedListForwardIterator<MySelf>;
    friend forward_iterator;

protected:
    Node  *m_pRoot = nullptr;
    Node  *m_tail  = nullptr;
    size_t m_size  = 0;
    Comp   m_comp;
    mutable shared_mutex m_mtx;

public:
    CircleLinkedList() {}

    // Copy Constructor
    CircleLinkedList(const CircleLinkedList &other) : m_pRoot(nullptr), m_tail(nullptr), m_size(0) {
        shared_lock<shared_mutex> lock(other.m_mtx);
        Node* curr = other.m_pRoot;
        for (size_t i = 0; i < other.m_size; ++i) {
            push_back(curr->getData(), curr->getRef());
            curr = curr->getNext();
        }
    }

    // Move Constructor
    CircleLinkedList(CircleLinkedList &&other) : m_pRoot(nullptr), m_tail(nullptr), m_size(0) {
        unique_lock<shared_mutex> lockOther(other.m_mtx);
        m_pRoot = std::exchange(other.m_pRoot, nullptr);
        m_tail  = std::exchange(other.m_tail,  nullptr);
        m_size  = std::exchange(other.m_size,  0);
    }

    // Copy Assignment
    CircleLinkedList& operator=(const CircleLinkedList &other) {
        if (this != &other) {
            while (m_size > 0) pop_front();
            shared_lock<shared_mutex> lock(other.m_mtx);
            Node* curr = other.m_pRoot;
            for (size_t i = 0; i < other.m_size; ++i) {
                push_back(curr->getData(), curr->getRef());
                curr = curr->getNext();
            }
        }
        return *this;
    }

    // Move Assignment
    CircleLinkedList& operator=(CircleLinkedList &&other) {
        if (this != &other) {
            while (m_size > 0) pop_front();
            unique_lock<shared_mutex> lockOther(other.m_mtx);
            m_pRoot = std::exchange(other.m_pRoot, nullptr);
            m_tail  = std::exchange(other.m_tail,  nullptr);
            m_size  = std::exchange(other.m_size,  0);
        }
        return *this;
    }

    // Destructor Seguro
    virtual ~CircleLinkedList() {
        unique_lock<shared_mutex> lock(m_mtx);
        if (m_tail) m_tail->setNext(nullptr); // rompe el círculo → lista lineal temporal
        Node* current = m_pRoot;
        while (current) {
            Node* next = current->getNext();
            delete current;
            current = next;
        }
        m_pRoot = nullptr;
        m_tail  = nullptr;
        m_size  = 0;
    }

    virtual void push_front(value_type value, Ref ref);
    virtual std::tuple<value_type, Ref> pop_front();
    virtual void push_back(value_type value, Ref ref);
    virtual void insert(const value_type &value, Ref ref);

    virtual value_type& operator[](size_t index);
    virtual size_t size() const;

    forward_iterator begin() { return forward_iterator(this, m_pRoot); }
    forward_iterator end()   { return forward_iterator(this, nullptr); }

    // ForEach
    // decidí reemplazar el range-based for por bucle un bucle contador por simplicidad
    template <typename Func, typename... Args>
    void ForEach(Func func, Args &&... args) {
        unique_lock<shared_mutex> lock(m_mtx);
        if (m_size == 0) return;
        Node* curr = m_pRoot;
        for (size_t i = 0; i < m_size; ++i) {
            func(curr->getDataRef(), std::forward<Args>(args)...);
            curr = curr->getNext();
        }
    }
};
