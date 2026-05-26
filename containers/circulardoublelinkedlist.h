#ifndef __CIRCULARDOUBLELINKEDLIST_H__
#define __CIRCULARDOUBLELINKEDLIST_H__

#include "doublelinkedlist.h"

template<typename T>
struct AscendingCDLLTrait  : BaseTrait<DLLNode<T>, less<T>>    {};
template<typename T>
struct DescendingCDLLTrait : BaseTrait<DLLNode<T>, greater<T>> {};

template<typename Trait>
class CircularDoubleLinkedList : public DoubleLinkedList<Trait> {
public:
    using value_type        = typename Trait::value_type;
    using Node              = typename Trait::Node;
    using Comp              = typename Trait::Comp;
    using MySelf            = CircularDoubleLinkedList<Trait>;

    using forward_iterator  = LinkedListForwardIterator <MySelf>;
    using backward_iterator = LinkedListBackwardIterator<MySelf>;
    friend forward_iterator;
    friend backward_iterator;

    forward_iterator  begin()  { return forward_iterator (this, static_cast<Node*>(this->m_pRoot)); }
    forward_iterator  end()    { return forward_iterator (this, static_cast<Node*>(this->m_pRoot)); }
    backward_iterator rbegin() { return backward_iterator(this, static_cast<Node*>(this->m_tail));  }
    backward_iterator rend()   { return backward_iterator(this, static_cast<Node*>(this->m_tail));  }

    CircularDoubleLinkedList() : DoubleLinkedList<Trait>() {}

    CircularDoubleLinkedList(const CircularDoubleLinkedList& other) : DoubleLinkedList<Trait>() {
        shared_lock<shared_mutex> lock(other.m_mtx);
        Node* curr = static_cast<Node*>(other.m_pRoot);
        for (size_t i = 0; i < other.m_size; ++i, curr = curr->getNext())
            push_back(curr->getData(), curr->getRef());
    }

    CircularDoubleLinkedList(CircularDoubleLinkedList&& other) : DoubleLinkedList<Trait>() {
        unique_lock<shared_mutex> lock(other.m_mtx);
        this->m_pRoot = exchange(other.m_pRoot, nullptr);
        this->m_tail  = exchange(other.m_tail,  nullptr);
        this->m_size  = exchange(other.m_size,  0);
    }

    CircularDoubleLinkedList& operator=(const CircularDoubleLinkedList& other) {
        if (this != &other) {
            clear();
            shared_lock<shared_mutex> lock(other.m_mtx);
            Node* curr = static_cast<Node*>(other.m_pRoot);
            for (size_t i = 0; i < other.m_size; ++i, curr = curr->getNext())
                push_back(curr->getData(), curr->getRef());
        }
        return *this;
    }

    CircularDoubleLinkedList& operator=(CircularDoubleLinkedList&& other) {
        if (this != &other) {
            clear();
            unique_lock<shared_mutex> lock(other.m_mtx);
            this->m_pRoot = exchange(other.m_pRoot, nullptr);
            this->m_tail  = exchange(other.m_tail,  nullptr);
            this->m_size  = exchange(other.m_size,  0);
        }
        return *this;
    }

    virtual ~CircularDoubleLinkedList() { clear(); }

    void clear() {
        unique_lock<shared_mutex> lock(this->m_mtx);
        if (this->m_size == 0) return;
        Node* tail = static_cast<Node*>(this->m_tail);
        tail->setNext(nullptr);                               // rompe next circular
        static_cast<Node*>(this->m_pRoot)->setPrev(nullptr); // rompe prev circular
        Node* act = static_cast<Node*>(this->m_pRoot);
        while (act) { Node* nxt = act->getNext(); delete act; act = nxt; }
        this->m_pRoot = this->m_tail = nullptr;
        this->m_size  = 0;
    }

    void push_back(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* n = new Node(value, ref);
        if (this->m_size == 0) {
            n->setNext(n); n->setPrev(n);
            this->m_pRoot = this->m_tail = n;
        } else {
            Node* tail = static_cast<Node*>(this->m_tail);
            Node* root = static_cast<Node*>(this->m_pRoot);
            tail->setNext(n);  n->setPrev(tail);
            n->setNext(root);  root->setPrev(n);
            this->m_tail = n;
        }
        ++this->m_size;
    }

    void push_front(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* n = new Node(value, ref);
        if (this->m_size == 0) {
            n->setNext(n); n->setPrev(n);
            this->m_pRoot = this->m_tail = n;
        } else {
            Node* root = static_cast<Node*>(this->m_pRoot);
            Node* tail = static_cast<Node*>(this->m_tail);
            n->setNext(root); n->setPrev(tail);
            root->setPrev(n); tail->setNext(n);
            this->m_pRoot = n;
        }
        ++this->m_size;
    }

    void insert(const value_type& value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* n = new Node(value, ref);
        if (this->m_size == 0) {
            n->setNext(n); n->setPrev(n);
            this->m_pRoot = this->m_tail = n;
            ++this->m_size;
            return;
        }
        Node* act = static_cast<Node*>(this->m_pRoot);
        for (size_t i = 0; i < this->m_size; ++i) {
            if (this->m_comp(value, act->getDataRef())) {
                Node* prev = act->getPrev();
                prev->setNext(n); n->setPrev(prev);
                n->setNext(act);  act->setPrev(n);
                if (act == this->m_pRoot) this->m_pRoot = n;
                ++this->m_size;
                return;
            }
            act = act->getNext();
        }
        Node* tail = static_cast<Node*>(this->m_tail);
        Node* root = static_cast<Node*>(this->m_pRoot);
        tail->setNext(n); n->setPrev(tail);
        n->setNext(root); root->setPrev(n);
        this->m_tail = n;
        ++this->m_size;
    }

    template<typename Func, typename... Args>
    void ForEach(Func func, Args&&... args) const {
        shared_lock<shared_mutex> lock(this->m_mtx);
        if (this->m_size == 0) return;
        Node* act = static_cast<Node*>(this->m_pRoot);
        for (size_t i = 0; i < this->m_size; ++i, act = act->getNext())
            func(act->getDataRef(), act->getRef(), forward<Args>(args)...);
    }

    template<typename Func, typename... Args>
    void ReverseForEach(Func func, Args&&... args) const {
        shared_lock<shared_mutex> lock(this->m_mtx);
        if (this->m_size == 0) return;
        Node* act = static_cast<Node*>(this->m_tail);
        for (size_t i = 0; i < this->m_size; ++i, act = act->getPrev())
            func(act->getDataRef(), act->getRef(), forward<Args>(args)...);
    }

    size_t size() const override {
        shared_lock<shared_mutex> lock(this->m_mtx);
        return this->m_size;
    }

protected:s
    void do_print(ostream& os) const override {
        size_t i = 0;
        ForEach([&](value_type& val, Ref ref) {
            if (i++ > 0) os << ",";
            os << "(" << val << "," << ref << ")";
        });
    }
};

#endif
