#ifndef __CIRCULARLINKEDLIST_H__
#define __CIRCULARLINKEDLIST_H__

#include "linkedlist.h"

template<typename T>
struct AscendingCLLTrait  : BaseTrait<LLNode<T>, less<T>>    {};
template<typename T>
struct DescendingCLLTrait : BaseTrait<LLNode<T>, greater<T>> {};

template<typename Trait>
class CircularLinkedList : public LinkedList<Trait> {
public:
    using value_type       = typename Trait::value_type;
    using Node             = typename Trait::Node;
    using Comp             = typename Trait::Comp;
    using MySelf           = CircularLinkedList<Trait>;

    using forward_iterator = LinkedListForwardIterator<MySelf>;
    friend forward_iterator;

    forward_iterator begin() { return forward_iterator(this, static_cast<Node*>(this->m_pRoot)); }
    forward_iterator end()   { return forward_iterator(this, static_cast<Node*>(this->m_pRoot)); }

    CircularLinkedList() : LinkedList<Trait>() {}

    CircularLinkedList(const CircularLinkedList& other) : LinkedList<Trait>() {
        shared_lock<shared_mutex> lock(other.m_mtx);
        Node* curr = static_cast<Node*>(other.m_pRoot);
        for (size_t i = 0; i < other.m_size; ++i, curr = curr->getNext())
            push_back(curr->getData(), curr->getRef());
    }

    CircularLinkedList(CircularLinkedList&& other) : LinkedList<Trait>() {
        unique_lock<shared_mutex> lock(other.m_mtx);
        this->m_pRoot = exchange(other.m_pRoot, nullptr);
        this->m_tail  = exchange(other.m_tail,  nullptr);
        this->m_size  = exchange(other.m_size,  0);
    }

    CircularLinkedList& operator=(const CircularLinkedList& other) {
        if (this != &other) {
            clear();
            shared_lock<shared_mutex> lock(other.m_mtx);
            Node* curr = static_cast<Node*>(other.m_pRoot);
            for (size_t i = 0; i < other.m_size; ++i, curr = curr->getNext())
                push_back(curr->getData(), curr->getRef());
        }
        return *this;
    }

    CircularLinkedList& operator=(CircularLinkedList&& other) {
        if (this != &other) {
            clear();
            unique_lock<shared_mutex> lock(other.m_mtx);
            this->m_pRoot = exchange(other.m_pRoot, nullptr);
            this->m_tail  = exchange(other.m_tail,  nullptr);
            this->m_size  = exchange(other.m_size,  0);
        }
        return *this;
    }

    virtual ~CircularLinkedList() { clear(); }

    void clear() {
        unique_lock<shared_mutex> lock(this->m_mtx);
        if (this->m_size == 0) return;
        static_cast<Node*>(this->m_tail)->setNext(nullptr); // rompe el ciclo
        Node* act = static_cast<Node*>(this->m_pRoot);
        while (act) { Node* nxt = act->getNext(); delete act; act = nxt; }
        this->m_pRoot = this->m_tail = nullptr;
        this->m_size  = 0;
    }

    void push_back(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* n = new Node(value, ref);
        if (this->m_size == 0) {
            n->setNext(n);
            this->m_pRoot = this->m_tail = n;
        } else {
            static_cast<Node*>(this->m_tail)->setNext(n);
            n->setNext(static_cast<Node*>(this->m_pRoot));
            this->m_tail = n;
        }
        ++this->m_size;
    }

    void push_front(value_type value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* n = new Node(value, ref, static_cast<Node*>(this->m_pRoot));
        if (this->m_size == 0) {
            n->setNext(n);
            this->m_pRoot = this->m_tail = n;
        } else {
            static_cast<Node*>(this->m_tail)->setNext(n);
            this->m_pRoot = n;
        }
        ++this->m_size;
    }

    void insert(const value_type& value, Ref ref) override {
        unique_lock<shared_mutex> lock(this->m_mtx);
        Node* n = new Node(value, ref);
        if (this->m_size == 0) {
            n->setNext(n);
            this->m_pRoot = this->m_tail = n;
            ++this->m_size;
            return;
        }
        Node* act  = static_cast<Node*>(this->m_pRoot);
        Node* prev = static_cast<Node*>(this->m_tail);
        for (size_t i = 0; i < this->m_size; ++i) {
            if (this->m_comp(value, act->getDataRef())) {
                prev->setNext(n);
                n->setNext(act);
                if (act == this->m_pRoot) this->m_pRoot = n;
                ++this->m_size;
                return;
            }
            prev = act;
            act  = act->getNext();
        }
        static_cast<Node*>(this->m_tail)->setNext(n);
        n->setNext(static_cast<Node*>(this->m_pRoot));
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

    size_t size() const override {
        shared_lock<shared_mutex> lock(this->m_mtx);
        return this->m_size;
    }

protected:
    void do_print(ostream& os) const override {
        size_t i = 0;
        ForEach([&](value_type& val, Ref ref) {
            if (i++ > 0) os << ",";
            os << "(" << val << "," << ref << ")";
        });
    }
};

#endif
