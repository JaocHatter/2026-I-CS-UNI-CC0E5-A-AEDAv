#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <utility>
#include <mutex>
#include <shared_mutex>
#include <cstddef>
#include <string>
#include "../types.h"
using namespace std;

template<typename Key, typename Value>
struct HashNode {
    Key       m_key;
    Value     m_value;
    HashNode* m_next;
    HashNode(Key k, Value v) : m_key(k), m_value(v), m_next(nullptr) {}
};

template<typename Key, typename Value, typename Hash = hash<Key>>
class HashTable {
public:
    using key_type   = Key;
    using value_type = Value;
    using Node       = HashNode<Key, Value>;
    using MySelf     = HashTable<Key, Value, Hash>;
    using pair_type  = pair<const Key&, Value&>;

private:
    static constexpr size_t DEFAULT_CAPACITY = 16;

    Node**               m_buckets;
    size_t               m_capacity;
    size_t               m_size;
    Hash                 m_hash;
    mutable shared_mutex m_mtx;

    size_t bucket_idx(const Key& k) const {
        return m_hash(k) % m_capacity;
    }

    void internal_clear() {
        for (size_t i = 0; i < m_capacity; ++i) {
            Node* cur = m_buckets[i];
            while (cur) {
                Node* nxt = cur->m_next;
                delete cur;
                cur = nxt;
            }
            m_buckets[i] = nullptr;
        }
        m_size = 0;
    }

public:
    class forward_iterator {
        const HashTable* m_table;
        size_t           m_bucket;
        Node*            m_node;

        void advance_to_valid() {
            while (m_bucket < m_table->m_capacity && !m_node) {
                ++m_bucket;
                if (m_bucket < m_table->m_capacity)
                    m_node = m_table->m_buckets[m_bucket];
            }
        }
    public:
        forward_iterator(const HashTable* t, size_t b, Node* n)
            : m_table(t), m_bucket(b), m_node(n) {
            if (!m_node && m_bucket < m_table->m_capacity)
                advance_to_valid();
        }

        pair_type operator*() const { return {m_node->m_key, m_node->m_value}; }

        forward_iterator& operator++() {
            m_node = m_node->m_next;
            if (!m_node) advance_to_valid();
            return *this;
        }

        bool operator==(const forward_iterator& o) const {
            return m_bucket == o.m_bucket && m_node == o.m_node;
        }
        bool operator!=(const forward_iterator& o) const { return !(*this == o); }
    };

    forward_iterator begin() const {
        return forward_iterator(this, 0, m_capacity ? m_buckets[0] : nullptr);
    }
    forward_iterator end() const {
        return forward_iterator(this, m_capacity, nullptr);
    }

    HashTable(size_t capacity = DEFAULT_CAPACITY)
        : m_buckets(new Node*[capacity]{}), m_capacity(capacity), m_size(0), m_hash() {}

    virtual ~HashTable() {
        internal_clear();
        delete[] m_buckets;
    }

    HashTable(const HashTable& other) {
        shared_lock lock(other.m_mtx);
        m_capacity = other.m_capacity;
        m_size     = 0;
        m_hash     = other.m_hash;
        m_buckets  = new Node*[m_capacity]{};
        for (size_t i = 0; i < m_capacity; ++i) {
            Node*  src = other.m_buckets[i];
            Node** dst = &m_buckets[i];
            while (src) {
                *dst = new Node(src->m_key, src->m_value);
                dst  = &(*dst)->m_next;
                src  = src->m_next;
                ++m_size;
            }
        }
    }

    HashTable(HashTable&& other) {
        unique_lock lock(other.m_mtx);
        m_buckets  = exchange(other.m_buckets,  new Node*[1]{});
        m_capacity = exchange(other.m_capacity, 1);
        m_size     = exchange(other.m_size,     0);
        m_hash     = move(other.m_hash);
    }

    Value& operator[](const Key& key) {
        unique_lock lock(m_mtx);
        size_t idx = bucket_idx(key);
        Node*  cur = m_buckets[idx];
        while (cur) {
            if (cur->m_key == key) return cur->m_value;
            cur = cur->m_next;
        }
        Node* fresh     = new Node(key, Value{});
        fresh->m_next   = m_buckets[idx];
        m_buckets[idx]  = fresh;
        ++m_size;
        return fresh->m_value;
    }

    bool contains(const Key& key) const {
        shared_lock lock(m_mtx);
        Node* cur = m_buckets[bucket_idx(key)];
        while (cur) {
            if (cur->m_key == key) return true;
            cur = cur->m_next;
        }
        return false;
    }

    bool remove(const Key& key) {
        unique_lock lock(m_mtx);
        size_t  idx = bucket_idx(key);
        Node** cur  = &m_buckets[idx];
        while (*cur) {
            if ((*cur)->m_key == key) {
                Node* del = *cur;
                *cur = del->m_next;
                delete del;
                --m_size;
                return true;
            }
            cur = &(*cur)->m_next;
        }
        return false;
    }

    size_t size() const {
        shared_lock lock(m_mtx);
        return m_size;
    }

    bool isEmpty() const {
        shared_lock lock(m_mtx);
        return m_size == 0;
    }

    string toString() const {
        shared_lock lock(m_mtx);
        ostringstream oss;
        oss << "{";
        bool first = true;
        for (size_t i = 0; i < m_capacity; ++i) {
            Node* cur = m_buckets[i];
            while (cur) {
                if (!first) oss << ",";
                oss << cur->m_key << ":" << cur->m_value;
                first = false;
                cur   = cur->m_next;
            }
        }
        oss << "}";
        return oss.str();
    }

    friend ostream& operator<<(ostream& os, const HashTable& h) {
        return os << h.toString();
    }

    friend istream& operator>>(istream& is, HashTable& h) {
        char ch;
        if (!(is >> ch) || ch != '{') { is.setstate(ios::failbit); return is; }
        while (is.peek() != '}') {
            Key k; Value v; char colon;
            if (!(is >> k >> colon >> v)) break;
            h[k] = v;
            if (is.peek() == ',') is >> ch;
        }
        is >> ch;
        return is;
    }
};

void HashTableDemo();

#endif // __HASHTABLE_H__
