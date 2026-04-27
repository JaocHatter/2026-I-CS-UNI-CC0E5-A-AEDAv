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

    // Operadores Stream de Salida y Entradad
    friend ostream& operator<<(ostream& os, const CircleLinkedList& list) {
        shared_lock<shared_mutex> lock(list.m_mtx);
        os << "[";
        Node* act = list.m_pRoot;
        for (size_t i = 0; i < list.m_size; ++i) {
            os << "(" << act->getData() << "," << act->getRef() << ")";
            if (i + 1 < list.m_size) os << ",";
            act = act->getNext();
        }
        os << "]";
        return os;
    }

    friend istream& operator>>(istream& is, CircleLinkedList& list) {
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

// push_front
template <typename Trait>
void CircleLinkedList<Trait>::push_front(value_type value, Ref ref) {
    unique_lock<shared_mutex> lock(m_mtx);
    Node* newNode = new Node(value, ref);
    if (m_size == 0) {
        m_pRoot = newNode;
        m_tail  = newNode;
        m_tail->setNext(m_pRoot);
    } else {
        newNode->setNext(m_pRoot);
        m_pRoot = newNode;
        m_tail->setNext(m_pRoot); 
    }
    m_size++;
}

// push_back
template <typename Trait>
void CircleLinkedList<Trait>::push_back(value_type value, Ref ref) {
    unique_lock<shared_mutex> lock(m_mtx);
    Node* newNode = new Node(value, ref);
    if (m_size == 0) {
        m_pRoot = newNode;
        m_tail  = newNode;
        m_tail->setNext(m_pRoot);
    } else {
        m_tail->setNext(newNode);
        m_tail = newNode;
        m_tail->setNext(m_pRoot);
    }
    m_size++;
}

// pop_front
// Este pop retorna la data y dirección del nodo eliminado
template <typename Trait>
std::tuple<typename CircleLinkedList<Trait>::value_type, Ref> CircleLinkedList<Trait>::pop_front() {
    unique_lock<shared_mutex> lock(m_mtx);
    if (!m_pRoot) throw runtime_error("lista vacia");

    Node* temp   = m_pRoot;
    auto  result = std::make_tuple(temp->getData(), temp->getRef());

    if (m_size == 1) {
        m_pRoot = nullptr;
        m_tail  = nullptr;
    } else {
        m_pRoot = temp->getNext();
        m_tail->setNext(m_pRoot); // reconecta el anillo con la nueva raiz
    }

    delete temp;
    m_size--;
    return result;
}

template <typename Trait>
typename CircleLinkedList<Trait>::value_type& CircleLinkedList<Trait>::operator[](size_t index) {
    shared_lock<shared_mutex> lock(m_mtx);
    if (index >= m_size) throw out_of_range("Indice fuera de rango");
    Node* act = m_pRoot;
    for (size_t i = 0; i < index; ++i) {
        act = act->getNext();
    }
    return act->getDataRef();
}

template <typename Trait>
size_t CircleLinkedList<Trait>::size() const {
    shared_lock<shared_mutex> lock(m_mtx);
    return m_size;
}

// insert
template <typename Trait>
void CircleLinkedList<Trait>::insert(const value_type &value, Ref ref) {
    unique_lock<shared_mutex> lock(m_mtx);
    Node* newNode = new Node(value, ref);

    if (m_pRoot == nullptr) {
        m_pRoot = newNode;
        m_tail  = newNode;
        newNode->setNext(m_pRoot);
        m_size++;
        return;
    }

    if (m_comp(value, m_pRoot->getData())) {
        newNode->setNext(m_pRoot);
        m_pRoot = newNode;
        m_tail->setNext(m_pRoot);
        m_size++;
        return;
    }

    Node* curr = m_pRoot;
    for (size_t i = 0; i < m_size - 1; ++i) {
        Node* nextNode = curr->getNext();
        if (m_comp(value, nextNode->getData())) {
            newNode->setNext(nextNode);
            curr->setNext(newNode);
            m_size++;
            return;
        }
        curr = nextNode;
    }

    // haciendo la lista circular...
    newNode->setNext(m_pRoot);
    m_tail->setNext(newNode);
    m_tail = newNode;
    m_size++;
}