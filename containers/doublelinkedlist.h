#ifndef __DOUBLELINKEDLIST_H__
#define __DOUBLELINKEDLIST_H__

#include "linkedlist.h"

template<typename T>
class DLLNode : public LLNodeBase<T, DLLNode<T>> {
    DLLNode<T>* m_pPrev;
public:
    DLLNode() : LLNodeBase<T, DLLNode<T>>(), m_pPrev(nullptr) {}
    DLLNode(T data, Ref ref, DLLNode<T>* next = nullptr, DLLNode<T>* prev = nullptr)
        : LLNodeBase<T, DLLNode<T>>(data, ref, next), m_pPrev(prev) {}
    virtual ~DLLNode() {}

    DLLNode<T>*  getPrev()            const { return m_pPrev; }
    void         setPrev(DLLNode<T>* p)     { m_pPrev = p;    }
    DLLNode<T>*& getPrevRef()               { return m_pPrev; }
};

template<typename T>
struct AscendingDLLTrait  : BaseTrait<DLLNode<T>, less<T>>    {};
template<typename T>
struct DescendingDLLTrait : BaseTrait<DLLNode<T>, greater<T>> {};

// ── DoubleLinkedList ──────────────────────────────────────────────────────────
template<typename Trait>
class DoubleLinkedList : public LinkedList<Trait> {
public:
    using value_type        = typename Trait::value_type;
    using Node              = typename Trait::Node;
    using Comp              = typename Trait::Comp;
    using MySelf            = DoubleLinkedList<Trait>;

    using forward_iterator  = LinkedListForwardIterator <MySelf>;
    using backward_iterator = LinkedListBackwardIterator<MySelf>;
    friend forward_iterator;
    friend backward_iterator;

    forward_iterator  begin()  { return forward_iterator (this, static_cast<Node*>(this->m_pRoot)); }
    forward_iterator  end()    { return forward_iterator (this, nullptr); }
    backward_iterator rbegin() { return backward_iterator(this, static_cast<Node*>(this->m_tail)); }
    backward_iterator rend()   { return backward_iterator(this, nullptr); }

    DoubleLinkedList() : LinkedList<Trait>() {}

    DoubleLinkedList(const DoubleLinkedList& other) : LinkedList<Trait>() {
        shared_lock<shared_mutex> lock(other.m_mtx);
        for (Node* c = static_cast<Node*>(other.m_pRoot); c; c = c->getNext())
            push_back(c->getData(), c->getRef());
    }

    DoubleLinkedList(DoubleLinkedList&& other) : LinkedList<Trait>(move(other)) {}

    DoubleLinkedList& operator=(const DoubleLinkedList& other) {
        if (this != &other) {
            while (this->m_size > 0) this->pop_front();
            shared_lock<shared_mutex> lock(other.m_mtx);
            for (Node* c = static_cast<Node*>(other.m_pRoot); c; c = c->getNext())
                push_back(c->getData(), c->getRef());
        }
        return *this;
    }

    DoubleLinkedList& operator=(DoubleLinkedList&& other) {
        if (this != &other) {
            while (this->m_size > 0) this->pop_front();
            unique_lock<shared_mutex> lock(other.m_mtx);
            this->m_pRoot = exchange(other.m_pRoot, nullptr);
            this->m_tail  = exchange(other.m_tail,  nullptr);
            this->m_size  = exchange(other.m_size,  0);
        }
        return *this;
    }

    template<typename Func, typename... Args>
    void ForEach(Func func, Args&&... args) {
        unique_lock<shared_mutex> lock(this->m_mtx);
        if (this->m_size == 0) return;
        for (auto& item : *this)
            func(item, forward<Args>(args)...);
    }

    void push_back(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* n = new Node(value, ref);
        if (this->m_size == 0) {
            this->m_pRoot = this->m_tail = n;
        } else {
            Node* tail = static_cast<Node*>(this->m_tail);
            tail->setNext(n);
            n->setPrev(tail);
            this->m_tail = n;
        }
        ++this->m_size;
    }

    void push_front(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* n = new Node(value, ref);
        if (this->m_size == 0) {
            this->m_pRoot = this->m_tail = n;
        } else {
            Node* root = static_cast<Node*>(this->m_pRoot);
            n->setNext(root);
            root->setPrev(n);
            this->m_pRoot = n;
        }
        ++this->m_size;
    }
};

#endif
