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
    using Node       = HeapNode<Trait>;
    
private:
    Vector<Trait> m_vec;
    Comp          m_comp;
    mutable shared_mutex m_mtx;
public:
    Heap() : m_vec(), m_comp() {}
    ~Heap() {}

    void heapifyUp(size_t index);
    void heapifyDown(size_t index);

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