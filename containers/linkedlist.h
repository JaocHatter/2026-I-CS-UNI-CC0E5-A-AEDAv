#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#include <iostream>
#include <cstddef>
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

template<typename T, typename Self>
class LLNodeBase {
    T     m_data;
    Ref   m_ref;
    Self* m_next;
public:
    using value_type = T;

    LLNodeBase() : m_data(T()), m_ref(Ref()), m_next(nullptr) {}
    LLNodeBase(T data, Ref ref, Self* next = nullptr)
        : m_data(data), m_ref(ref), m_next(next) {}
    virtual ~LLNodeBase() {}

    T      getData()    const { return m_data;  }
    T&     getDataRef()       { return m_data;  }
    void   setData(T data)    { m_data = data;  }
    Ref    getRef()     const { return m_ref;   }
    void   setRef(Ref ref)    { m_ref = ref;    }
    Self*  getNext()    const { return m_next;  }
    Self*& getNextRef()       { return m_next;  }
    void   setNext(Self* n)   { m_next = n;     }
};

template<typename T>
class LLNode : public LLNodeBase<T, LLNode<T>> {
public:
    LLNode() : LLNodeBase<T, LLNode<T>>() {}
    LLNode(T data, Ref ref, LLNode<T>* next = nullptr)
        : LLNodeBase<T, LLNode<T>>(data, ref, next) {}
    virtual ~LLNode() {}
};

template<typename T>
struct AscendingLinkedListTrait : public BaseTrait<LLNode<T>, less<T>> {};

template<typename T>
struct DescendingLinkedListTrait : public BaseTrait<LLNode<T>, greater<T>> {};


template<typename Container>
class LinkedListForwardIterator
    : public general_iterator<Container, LinkedListForwardIterator<Container>> {
public:
    using MySelf = LinkedListForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;

    MySelf operator++() {
        if (this->m_pNode)
            this->m_pNode = this->m_pNode->getNext();
        return *this;
    }
};

template<typename Container>
class LinkedListBackwardIterator
    : public general_iterator<Container, LinkedListBackwardIterator<Container>> {
public:
    using MySelf = LinkedListBackwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;

    MySelf operator++() {
        if (this->m_pNode)
            this->m_pNode = this->m_pNode->getPrev();
        return *this;
    }
};

template<typename Trait>
class LinkedList {
public:
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Comp       = typename Trait::Comp;
    using MySelf     = LinkedList<Trait>;

    using forward_iterator = LinkedListForwardIterator<MySelf>;
    friend forward_iterator;

protected:
    Node*  m_pRoot = nullptr;
    Node*  m_tail  = nullptr;
    size_t m_size  = 0;
    Comp   m_comp;
    mutable shared_mutex m_mtx;

private:
    void internal_insert(Node*& pPrev, const value_type& value, Ref ref);

public:
    LinkedList() {}

    LinkedList(const LinkedList& other) : m_pRoot(nullptr), m_tail(nullptr), m_size(0) {
        shared_lock<shared_mutex> lock(other.m_mtx);
        for (Node* curr = other.m_pRoot; curr != nullptr; curr = curr->getNext())
            push_back(curr->getData(), curr->getRef());
    }

    LinkedList(LinkedList&& other) : m_pRoot(nullptr), m_tail(nullptr), m_size(0) {
        unique_lock<shared_mutex> lock(other.m_mtx);
        m_pRoot = exchange(other.m_pRoot, nullptr);
        m_tail  = exchange(other.m_tail,  nullptr);
        m_size  = exchange(other.m_size,  0);
    }

    LinkedList& operator=(const LinkedList& other) {
        if (this != &other) {
            while (m_size > 0) pop_front();
            shared_lock<shared_mutex> lock(other.m_mtx);
            for (Node* curr = other.m_pRoot; curr != nullptr; curr = curr->getNext())
                push_back(curr->getData(), curr->getRef());
        }
        return *this;
    }

    LinkedList& operator=(LinkedList&& other) {
        if (this != &other) {
            while (m_size > 0) pop_front();
            unique_lock<shared_mutex> lock(other.m_mtx);
            m_pRoot = exchange(other.m_pRoot, nullptr);
            m_tail  = exchange(other.m_tail,  nullptr);
            m_size  = exchange(other.m_size,  0);
        }
        return *this;
    }

    virtual ~LinkedList() {
        unique_lock<shared_mutex> lock(m_mtx);
        Node* cur = m_pRoot;
        while (cur) { Node* nxt = cur->getNext(); delete cur; cur = nxt; }
        m_pRoot = m_tail = nullptr;
        m_size  = 0;
    }

    virtual void push_front(value_type value, Ref ref);
    virtual tuple<value_type, Ref> pop_front();
    virtual void push_back(value_type value, Ref ref);
    virtual tuple<value_type, Ref> pop_back();
    virtual void insert(const value_type& value, Ref ref);
    virtual value_type& operator[](size_t index);
    virtual size_t size() const;

    forward_iterator begin() { return forward_iterator(this, m_pRoot); }
    forward_iterator end()   { return forward_iterator(this, nullptr); }

    template<typename Func, typename... Args>
    void ForEach(Func func, Args&&... args) {
        unique_lock<shared_mutex> lock(m_mtx);
        if (m_size == 0) return;
        for (auto& item : *this)
            func(item, forward<Args>(args)...);
    }

    friend ostream& operator<<(ostream& os, const LinkedList& list) {
        shared_lock<shared_mutex> lock(list.m_mtx);
        os << "[";
        list.do_print(os);
        os << "]";
        return os;
    }

    friend istream& operator>>(istream& is, LinkedList& list) {
        char ch;
        if (!(is >> ch) || ch != '[') { is.clear(ios_base::failbit); return is; }
        value_type val; Ref ref; char comma, parenClose;
        while (is >> ch && ch != ']') {
            if (ch == '(')
                if (is >> val >> comma >> ref >> parenClose)
                    if (comma == ',' && parenClose == ')')
                        list.push_back(val, ref);
        }
        return is;
    }

protected:
    virtual void Print(ostream& os) const {
        Node* act = m_pRoot;
        while (act) {
            os << "(" << act->getData() << "," << act->getRef() << ")";
            if (act->getNext()) os << ",";
            act = act->getNext();
        }
    }
};

template<typename Trait>
void LinkedList<Trait>::internal_insert(Node*& pPrev, const value_type& value, Ref ref) {
    if (!pPrev || m_comp(value, pPrev->getDataRef())) {
        pPrev = new Node(value, ref, pPrev);
        ++m_size;
        if (!pPrev->getNext()) m_tail = pPrev;
        return;
    }
    internal_insert(pPrev->getNextRef(), value, ref);
}

template<typename Trait>
void LinkedList<Trait>::insert(const value_type& value, Ref ref) {
    unique_lock<shared_mutex> lock(m_mtx);
    internal_insert(m_pRoot, value, ref);
    if (m_size == 1) {
        m_tail = m_pRoot;
    } else {
        Node* act = m_pRoot;
        while (act && act->getNext()) act = act->getNext();
        m_tail = act;
    }
}

template<typename Trait>
void LinkedList<Trait>::push_front(value_type value, Ref ref) {
    unique_lock<shared_mutex> lock(m_mtx);
    m_pRoot = new Node(value, ref, m_pRoot);
    if (m_size == 0) m_tail = m_pRoot;
    ++m_size;
}

template<typename Trait>
void LinkedList<Trait>::push_back(value_type value, Ref ref) {
    unique_lock<shared_mutex> lock(m_mtx);
    Node* n = new Node(value, ref);
    if (m_size == 0) { m_pRoot = m_tail = n; }
    else             { m_tail->setNext(n); m_tail = n; }
    ++m_size;
}

template<typename Trait>
tuple<typename LinkedList<Trait>::value_type, Ref> LinkedList<Trait>::pop_front() {
    unique_lock<shared_mutex> lock(m_mtx);
    if (!m_pRoot) throw runtime_error("Lista vacia");
    Node* tmp = m_pRoot;
    auto  res = make_tuple(tmp->getData(), tmp->getRef());
    m_pRoot   = m_pRoot->getNext();
    delete tmp; --m_size;
    if (m_size == 0) m_tail = nullptr;
    return res;
}

template<typename Trait>
tuple<typename LinkedList<Trait>::value_type, Ref> LinkedList<Trait>::pop_back() {
    unique_lock<shared_mutex> lock(m_mtx);
    if (!m_pRoot) throw runtime_error("Lista vacia");
    tuple<value_type, Ref> res;
    if (m_pRoot == m_tail) {
        res = make_tuple(m_pRoot->getData(), m_pRoot->getRef());
        delete m_pRoot; m_pRoot = m_tail = nullptr;
    } else {
        Node* act = m_pRoot;
        while (act->getNext() != m_tail) act = act->getNext();
        res = make_tuple(m_tail->getData(), m_tail->getRef());
        delete m_tail; m_tail = act; m_tail->setNext(nullptr);
    }
    --m_size;
    return res;
}

template<typename Trait>
typename LinkedList<Trait>::value_type& LinkedList<Trait>::operator[](size_t index) {
    shared_lock<shared_mutex> lock(m_mtx);
    if (index >= m_size) throw out_of_range("Indice fuera de rango");
    Node* act = m_pRoot;
    for (size_t i = 0; i < index; ++i) act = act->getNext();
    return act->getDataRef();
}

template<typename Trait>
size_t LinkedList<Trait>::size() const {
    shared_lock<shared_mutex> lock(m_mtx);
    return m_size;
}

#endif // __LINKEDLIST_H__
