#ifndef __HEAP_H__
#define __HEAP_H__

#include <iostream>
#include <cstddef> // size_t
#include <string>
#include <sstream>
#include <stdexcept>
#include <mutex>
#include <utility>
#include <tuple>
#include "util.h"
#include "../types.h"
#include "traits.h"
using namespace std;

template<typename T>
class HeapNode{
public:
    using value_type = T;
    using MySelf     = HeapNode<T>;
private:
    T m_data;
    Ref        m_ref;
public:
    HeapNode() : m_data(T()), m_ref(0) {}
    HeapNode(T data, Ref ref) : m_data(data), m_ref(ref) {}
    T&  getDataRef()      { return m_data; }
    T   getData()   const { return m_data; }
    Ref getRef()    const { return m_ref; }
    ~HeapNode() {}
};

template<typename T>
struct MinHeapTrait : public BaseTrait<HeapNode<T>, less<T>> {};

template<typename T>
struct MaxHeapTrait : public BaseTrait<HeapNode<T>, greater<T>> {};

template<typename Trait>
class Heap{
public:
    using value_type = typename Trait::value_type;
    using Comp       = typename Trait::Comp;
    using MySelf     = Heap<Trait>;
    using Node       = typename Trait::Node;
    
private:
    Node*                m_data;
    size_t               m_size;
    size_t               m_capacity;
    Comp                 m_comp;
    mutable shared_mutex m_mtx;

    void resize() {
        m_capacity = (m_capacity < 10) ? m_capacity + 10 : m_capacity * 2;
        Node* fresh = new Node[m_capacity];
        for (size_t i = 0; i < m_size; ++i)
            fresh[i] = m_data[i];
        delete[] m_data;
        m_data = fresh;
    }

    void heapifyUp(size_t index) {
        while (index > 0) {
            size_t parent = (index - 1) / 2;
            if (m_comp(m_data[index].getData(), m_data[parent].getData())) {
                swap(m_data[index], m_data[parent]);
                index = parent;
            } else break;
        }
    }

    void heapifyDown(size_t index) {
        while (true) {
            size_t best  = index;
            size_t left  = 2 * index + 1;
            size_t right = 2 * index + 2;
            if (left  < m_size && m_comp(m_data[left].getData(),  m_data[best].getData())) best = left;
            if (right < m_size && m_comp(m_data[right].getData(), m_data[best].getData())) best = right;
            if (best == index) break;
            swap(m_data[index], m_data[best]);
            index = best;
        }
    }

public:
    Heap(size_t capacity = 10)
        : m_data(new Node[capacity]), m_size(0), m_capacity(capacity), m_comp() {}
    
    virtual ~Heap() { delete[] m_data; }

    Heap(const Heap& other) {
        shared_lock lock(other.m_mtx);
        m_capacity = other.m_capacity;
        m_size     = other.m_size;
        m_comp     = other.m_comp;
        m_data     = new Node[m_capacity];
        for (size_t i = 0; i < m_size; ++i)
            m_data[i] = other.m_data[i];
    }

    Heap(Heap&& other) {
        unique_lock lock(other.m_mtx);
        m_data     = exchange(other.m_data,     nullptr);
        m_size     = exchange(other.m_size,     0);
        m_capacity = exchange(other.m_capacity, 0);
        m_comp     = move(other.m_comp);
    }

    void insert(value_type value, Ref ref = 0) {
        unique_lock lock(m_mtx);
        if (m_size == m_capacity) resize();
        m_data[m_size++] = Node(value, ref);
        heapifyUp(m_size - 1);
    }

    void extract() {
        unique_lock lock(m_mtx);
        if (m_size == 0) throw out_of_range("Heap::extract — heap is empty");
        swap(m_data[0], m_data[m_size - 1]);
        --m_size;
        if (m_size > 0) heapifyDown(0);
    }

    Node peek() const {
        shared_lock lock(m_mtx);
        if (m_size == 0) throw out_of_range("Heap::peek — heap is empty");
        return m_data[0];
    }

    bool isEmpty() const {
        shared_lock lock(m_mtx);
        return m_size == 0;
    }

    size_t size() const {
        shared_lock lock(m_mtx);
        return m_size;
    }

    string toString() const {
        shared_lock lock(m_mtx);
        ostringstream oss;
        oss << "[";
        for (size_t i = 0; i < m_size; ++i) {
            if (i > 0) oss << ",";
            oss << "(" << m_data[i].getData() << "," << m_data[i].getRef() << ")";
        }
        oss << "]";
        return oss.str();
    }

    friend ostream& operator<<(ostream& os, const Heap& h) {
        return os << h.toString();
    }

    friend istream& operator>>(istream& is, Heap& h) {
        char ch;
        if (!(is >> ch) || ch != '[') { is.setstate(ios::failbit); return is; }
        while (is.peek() != ']' && is >> ch) {
            if (ch == '(') {
                value_type val;
                Ref        ref;
                char       comma, close;
                if (is >> val >> comma >> ref >> close)
                    h.insert(val, ref);
            }
        }
        is >> ch;
        return is;
    }
};




#endif // __HEAP_H__