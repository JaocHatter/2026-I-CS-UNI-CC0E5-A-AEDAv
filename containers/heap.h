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
    Heap() : m_vec(), m_comp() {}
    ~Heap() {}
    
    void insert(value_type value, Ref ref);
    
    // Extrae el elemento de mayor o menor prioridad (depende del heap)
    void extract(); 
    
    // Obtiene el elemento de mayor o menor prioridad (depende del heap) 
    // sin removerlo
    Node peek();
    
    bool isEmpty();
    size_t size();
    string toString();
};




#endif // __HEAP_H__