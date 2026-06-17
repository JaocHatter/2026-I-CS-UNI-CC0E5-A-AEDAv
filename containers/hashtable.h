#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <iostream>
#include <stdexcept>
#include <shared_mutex>
#include "BinaryTreeAVL.h"
#include "traits.h"
using namespace std;

// =====================================================================
// KVPair<Key, Value>
//
// value_type que vive dentro del AVL. Los operadores de comparacion
// miran solo la Key — el Trait/Comparador los usa para ordenar el arbol.
// =====================================================================
template <typename Key, typename Value>
class KVPair {
public:
    Key   m_key;
    Value m_value;

    KVPair() : m_key(), m_value() {}
    KVPair(const Key& k, const Value& v) : m_key(k), m_value(v) {}

    bool operator<(const KVPair& o)  const { return m_key <  o.m_key; }
    bool operator>(const KVPair& o)  const { return m_key >  o.m_key; }
    bool operator==(const KVPair& o) const { return m_key == o.m_key; }
};

template <typename K, typename V>
ostream& operator<<(ostream& os, const KVPair<K,V>& p) {
    return os << p.m_key << ":" << p.m_value;
}

template <typename K, typename V>
istream& operator>>(istream& is, KVPair<K,V>& p) {
    char colon;
    return is >> p.m_key >> colon >> p.m_value;
}

// =====================================================================
// KVKeyLess<Key,Value>
//
// Comparador que el Trait entrega al AVL. Compara KVPair unicamente
// por la Key — garantiza que el arbol quede ordenado por clave sin
// depender de operator< del par completo.
// =====================================================================
template <typename Key, typename Value>
struct KVKeyLess {
    bool operator()(const KVPair<Key,Value>& a,
                    const KVPair<Key,Value>& b) const {
        return a.m_key < b.m_key;
    }
};

// =====================================================================
// HashTable<Key, Value>
//
// Trait = BaseTrait< AVLNode<KVPair>, KVKeyLess >
// Hereda BinaryTreeAVL con ese Trait: el AVL ES el contenedor, ordenado
// por Key via KVKeyLess. No hay array de buckets ni funcion hash.
// Hereda balanceo, concurrencia.
// =====================================================================
template <typename Key, typename Value>
class HashTable
    : public BinaryTreeAVL<BaseTrait<AVLNode<KVPair<Key,Value>>,
                                     KVKeyLess<Key,Value>>> {
public:
    using Pair  = KVPair<Key, Value>;
    using Trait = BaseTrait<AVLNode<Pair>, KVKeyLess<Key,Value>>;
    using Base  = BinaryTreeAVL<Trait>;
    using Node  = typename Trait::Node;   // AVLNode<Pair>
    using value_type = Pair;
    using Base::insert;

private:
    // Recorre el AVL sin lock (el llamador lo sostiene).
    // Usa this->m_comp (KVKeyLess) para seguir el mismo criterio de
    // avl_insert: branch = !m_comp(node, probe).
    Node* find_node_unsafe(const Key& key) const {
        Pair probe{key, Value{}};
        Node* n = this->m_pRoot;
        while (n) {
            if (!this->m_comp(n->m_data, probe) &&
                !this->m_comp(probe, n->m_data)) return n;
            n = n->m_pChild[!this->m_comp(n->m_data, probe)];
        }
        return nullptr;
    }

public:
    HashTable() = default;

    // find-or-insert: devuelve referencia al valor (lo crea si no existe).
    Value& operator[](const Key& key) {
        unique_lock lock(this->m_mtx);
        if (Node* n = find_node_unsafe(key)) return n->m_data.m_value;
        this->internal_insert(this->m_pRoot, Pair{key, Value{}});
        return find_node_unsafe(key)->m_data.m_value;
    }

    // at — lanza out_of_range si la key no existe.
    const Value& at(const Key& key) const {
        shared_lock lock(this->m_mtx);
        const Node* n = find_node_unsafe(key);
        if (!n) throw out_of_range("HashTable::at — key no existe");
        return n->m_data.m_value;
    }

    bool contains(const Key& key) const {
        shared_lock lock(this->m_mtx);
        return find_node_unsafe(key) != nullptr;
    }

    // insert(key, value) — inserta o sobreescribe el valor existente.
    void insert(const Key& key, const Value& value) {
        unique_lock lock(this->m_mtx);
        if (Node* n = find_node_unsafe(key)) { n->m_data.m_value = value; return; }
        this->internal_insert(this->m_pRoot, Pair{key, value});
    }

    void remove(const Key& key) {
        Base::remove(Pair{key, Value{}});
    }
};

// =====================================================================
// Structured bindings: for (const auto& [k, v] : table)
// =====================================================================
namespace std {

template <typename K, typename V>
struct tuple_size<::KVPair<K,V>> : integral_constant<size_t, 2> {};

template <typename K, typename V>
struct tuple_element<0, ::KVPair<K,V>> { using type = K; };

template <typename K, typename V>
struct tuple_element<1, ::KVPair<K,V>> { using type = V; };

}

template <size_t I, typename K, typename V>
auto& get(KVPair<K,V>& p) {
    if constexpr (I == 0) return p.m_key;
    else                  return p.m_value;
}

template <size_t I, typename K, typename V>
const auto& get(const KVPair<K,V>& p) {
    if constexpr (I == 0) return p.m_key;
    else                  return p.m_value;
}

void HashTableDemo();

#endif // __HASHTABLE_H__
