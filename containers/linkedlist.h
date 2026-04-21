#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__

#include <iostream>
#include <cstddef> // size_t
#include <string>
#include <sstream>
#include <shared_mutex> // shared_mutex
#include "general_iterator.h"
#include "util.h"
#include "../types.h"
using namespace std;

// Forward iterator
template <typename Container>
class LinkedListForwardIterator : public general_iterator<Container, LinkedListForwardIterator<Container>>{
    using MySelf = LinkedListForwardIterator<Container>;
    using Parent = general_iterator<Container, MySelf>;
    using Parent::Parent;
    // TODO: Completar el operator++
    public:
        MySelf& operator++(){
            this->m_pNode = this->m_pNode->getNext();
            return *this;
        };
};

// Linked List Node
template <typename T>
class LLNode{
    using Node = LLNode<T>;
private:
    T   m_data;
    Node *m_next;
public:
    LLNode() : m_data(T()), m_next(nullptr) {}
    LLNode(T data) : m_data(data), m_next(nullptr) {}
    LLNode(T data, Node *next) : m_data(data), m_next(next) {}
    virtual ~LLNode() {}

    T      getData() const { return m_data; }
    T&     getDataRef()    { return m_data; }
    void   setData(T data) { m_data = data; }
    Node*  getNext() const { return m_next; }
    Node*& getNextRef()    { return m_next; }
    void   setNext(Node *next) { m_next = next; }
};

template <typename T>
struct AscendingLinkedListTrait{
    using value_type = T;
    using Node = LLNode<T>;
    using Comp = less<T>;
};

template <typename T>
struct DescendingLinkedListTrait{
    using value_type = T;
    using Node = LLNode<T>;
    using Comp = greater<T>;
};

template <typename Trait>
class LinkedList{
public:
    using value_type = typename Trait::value_type;
    using Node       = typename Trait::Node;
    using Comp       = typename Trait::Comp;
    using MySelf     = LinkedList<Trait>;

    using forward_iterator = LinkedListForwardIterator<MySelf>;
    // friend forward_iterator;

private:
    Node *m_pRoot = nullptr;
    Node *m_tail = nullptr;
    size_t m_size = 0;
    Comp   m_comp;
    mutable shared_mutex m_mtx;
public:
    LinkedList() {}
    LinkedList(const LinkedList &other){ // Copy constructor
        Node* pt_node = other.m_pRoot;
        // Copiamos cada uno de los nodos desde la raiz
        while(pt_node) {
            Node* tmp_node = new Node(pt_node->getData(), nullptr);
            if(!m_pRoot){
                //llegamos al final
                m_pRoot = m_tail = tmp_node;
            } else {
                m_tail->setNext(tmp_node);
                m_tail = tmp_node;
            }
            m_size++;
            //continuamos
            pt_node = pt_node->getNext();
        }
    }
    LinkedList(LinkedList &&other){ // Move constructor
        // Bloqueamos el objeto de origen para asegurar que nadie lo modifique mientras ejecutamos el constructor move
        unique_lock<shared_mutex> lock(other.m_mtx);

        // Transferimos la propiedad de los recursos
        m_pRoot = other.m_pRoot;
        m_tail  = other.m_tail; 
        m_size  = other.m_size; 
        m_comp  = move(other.m_comp);

        // Dejamos al objeto original en un estado válido pero vacío
        // esto es vital para que su destructor no borre la memoria que acabamos de tomar
        other.m_pRoot = nullptr;
        other.m_tail  = nullptr;
        other.m_size  = 0;
    }
    LinkedList& operator=(const LinkedList &other){ // Copy assignment operator
    }
    LinkedList& operator=(LinkedList &&other){ // Move assignment operator
        if (this == &other) return *this;

        // Bloqueo atómico de ambos mutexes para evitar condiciones de carrera y deadlocks
        scoped_lock lock(m_mtx, other.m_mtx);

        // Liberar los recursos que este objeto posee actualmente
        Node* current = m_pRoot;
        while (current) {
            Node* next = current->getNext();
            delete current;
            current = next;
        }

        // transferencia de propied
        m_pRoot = other.m_pRoot;
        m_tail  = other.m_tail; 
        m_size  = other.m_size; 
        m_comp  = std::move(other.m_comp);

        // Resetear el objeto origen 
        // Es vital dejar 'other' en un estado vacío para que su destructor no borre la memoria
        other.m_pRoot = nullptr;
        other.m_tail  = nullptr;
        other.m_size  = 0;      

        return *this;
    }
    
    virtual        ~LinkedList() {
        // Se usa unique_lock porque la destrucción es una operación de escritura/modificación. solo un hilo accede
        unique_lock<shared_mutex> lock(m_mtx); 
        
        // Puntero auxiliar para recorrer la lista comenzando desde la raíz.
        Node *pCurr = m_pRoot;
        
        // Bucle de liberación de memoria.
        while(pCurr) {
            // Guardamos la referencia al siguiente nodo antes de eliminar el actual.
            Node *pNext = pCurr->getNext();
            delete pCurr;
            pCurr = pNext;
        }
         
        m_pRoot = m_tail = nullptr;
        m_size = 0;
    }
    virtual void    push_front(value_type value, Ref ref);
    virtual void    pop_front();
    virtual void    push_back(value_type value, Ref ref);
    virtual void    pop_back();
private:
            void    internal_insert(Node* &pParent, const value_type &value, Ref ref);
public:
    virtual void    insert(const value_type &value, Ref ref);
    
    virtual value_type& operator[](size_t index);
    virtual size_t  size() const;
    virtual string  toString() const;

    forward_iterator begin() { return forward_iterator(this, m_pRoot); }
    forward_iterator end()   { return forward_iterator(this, nullptr); }

    // Agregar Foreach
    template <typename Func, typename... Args>
    void ForEach(Func func, Args &&...  args){
        unique_lock<shared_mutex> lock(m_mtx);
        ::ForEach(begin(), end(), func, std::forward<Args>(args)... );
    }
};

template <typename T>
void LinkedList<T>::internal_insert(Node* &pPr  ev, const value_type &value, Ref ref){
    if(!pPrev || m_comp(value, pPrev->getDataRef())){
        pPrev = new Node(value, ref, pPrev);
        m_size++;
        if(pPrev == m_pRoot)
            m_tail = pPrev;
        return;
    }
    internal_insert(pPrev->getNextRef(), value, ref);
}

template <typename T>
void LinkedList<T>::insert(const value_type &value, Ref ref){
    internal_insert(m_pRoot, value, ref);
}

template <typename T>
void LinkedList<T>::push_front(value_type value, Ref ref){
    // Aseguramos que solo 1 hilo pueda acceder a los recursos cuando la funcion es llamada
    unique_lock<shared_mutex> lock(m_mtx);
    m_pRoot = new Node(value, m_pRoot);
    if (m_size == 0) {
        m_tail = m_pRoot;
    } 
    m_size++;
}

template <typename T>
void LinkedList<T>::pop_front(){
    unique_lock<shared_mutex> lock(m_mtx);
    // verifiquemos que haya algo que eliminar
    if (!m_pRoot) return;
        
    Node* pTemp = m_pRoot;
    m_pRoot = m_pRoot->getNext();
    delete pTemp;
        
    m_size--;
    if (m_size == 0) m_tail = nullptr;
}

template <typename T> 
void LinkedList<T>::push_back(value_type value, Ref ref) {
    unique_lock<shared_mutex> lock(m_mtx);
    Node *tail = new Node(value, ref, nullptr);
    if (m_pRoot == nullptr) {
        m_pRoot = tail;
        m_tail = tail;
    } else {
        m_tail->setNext(tail);
        m_tail = tail;
    }
    m_size++;
}

template <typename T> 
void LinkedList<T>::pop_back() {
    unique_lock<shared_mutex> lock(m_mtx);

    if (!m_pRoot)
        return;

    // Manejo del caso especial donde solo existe un elemento.
    if (m_pRoot == m_tail) {
        delete m_pRoot;     
        m_pRoot = nullptr;
        m_tail = nullptr;
        m_size--;
        return;
    }

    Node *pretail = m_pRoot;
    while (pretail->getNextRef() != m_tail) {
        pretail = pretail->getNextRef();
    }

    // Eliminación del nodo final y actualización de la estructura.
    delete m_tail;        
    m_tail = pretail;
    m_tail->setNext(nullptr);
    m_size--;
}

#endif // __LINKEDLIST_H__