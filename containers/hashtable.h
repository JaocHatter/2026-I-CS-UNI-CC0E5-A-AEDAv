#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <iostream>
#include <stdexcept>
#include <shared_mutex>
#include <tuple>
#include "BinaryTreeAVL.h"
using namespace std;

// =====================================================================
// KVPair<Key, Value, Comp>
//
// value_type del arbol. Comp define el orden por Key; el AVL usa
// operator< de KVPair, que delega en Comp, para ordenarse.
// =====================================================================
template <typename Key, typename Value, typename Comp = less<Key>>
struct KVPair {
    Key           m_key;
    mutable Value m_value;

    KVPair() : m_key(), m_value() {}
    KVPair(const Key& k, const Value& v = Value{}) : m_key(k), m_value(v) {}

    bool operator< (const KVPair& o) const { return Comp{}(m_key, o.m_key); }
    bool operator> (const KVPair& o) const { return Comp{}(o.m_key, m_key); }
    bool operator==(const KVPair& o) const { return !(*this < o) && !(o < *this); }

    friend ostream& operator<<(ostream& os, const KVPair& p) { return os << p.m_key << ":" << p.m_value; }
    friend istream& operator>>(istream& is, KVPair& p) { char c; return is >> p.m_key >> c >> p.m_value; }
};

// =====================================================================
// Structured bindings: for (const auto& [k, v] : table)
// =====================================================================
namespace std {
    template<typename K, typename V, typename C>
    struct tuple_size<KVPair<K,V,C>> : integral_constant<size_t, 2> {};

    template<typename K, typename V, typename C>
    struct tuple_element<0, KVPair<K,V,C>> { using type = const K; };

    template<typename K, typename V, typename C>
    struct tuple_element<1, KVPair<K,V,C>> { using type = V; };
}

template<size_t I, typename K, typename V, typename C>
decltype(auto) get(KVPair<K,V,C>& p)
    { if constexpr (I==0) return (const K&)p.m_key; else return p.m_value; }

template<size_t I, typename K, typename V, typename C>
decltype(auto) get(const KVPair<K,V,C>& p)
    { if constexpr (I==0) return (const K&)p.m_key; else return (const V&)p.m_value; }

// =====================================================================
// HashTableTrait<Key, Value, Comp>
//
// Ficha tecnica que HashTable recibe como parametro de template.
// Agrupa los tres tipos que definen el comportamiento del mapa:
//   Key   — tipo de la clave
//   Value — tipo del valor asociado
//   Comp  — criterio de orden entre claves (por defecto: less<Key>)
// =====================================================================
template <typename _Key, typename _Value, typename _Comp = less<_Key>>
struct HashTableTrait {
    using Key   = _Key;
    using Value = _Value;
    using Comp  = _Comp;
};

// =====================================================================
// HashTable<Trait>
//
// AVL Map: el arbol AVL es el contenedor; los pares quedan ordenados
// por Key via Comp. Hereda balanceo O(log n) y Big Five del arbol.
//
// m_mtx protege las operaciones compuestas (buscar + insertar) de
// condiciones de carrera entre hilos.
// =====================================================================
template <typename Trait>
class HashTable {
public:
    using Key   = typename Trait::Key;
    using Value = typename Trait::Value;
    using Comp  = typename Trait::Comp;
    using Pair  = KVPair<Key, Value, Comp>;
    using Tree  = BinaryTreeAVL<AscendingTrait<AVLNode<Pair>>>;
    using Node  = AVLNode<Pair>;

private:
    Tree                 m_tree;
    mutable shared_mutex m_mtx;

    // Recorre el AVL sin lock (el llamador lo sostiene).
    // Usa KVPair::operator< (que usa Comp) para seguir la convencion
    // del arbol: branch = !(node < probe).
    Node* buscar(const Key& key) const {
        Pair probe{key};
        Node* n = m_tree.getRoot();
        while (n) {
            if (!(n->m_data < probe) && !(probe < n->m_data)) return n;
            n = n->m_pChild[!(n->m_data < probe)];
        }
        return nullptr;
    }

public:
    HashTable() = default;

    HashTable(const HashTable& o) {
        shared_lock<shared_mutex> lock(o.m_mtx);
        for (const Pair& p : o.m_tree.snapshot())
            m_tree.insert(p);
    }

    HashTable(HashTable&& o) : m_tree(move(o.m_tree)) {}

    ~HashTable() = default;

    // find-or-insert: crea la entrada con Value{} si no existe.
    Value& operator[](const Key& key) {
        unique_lock<shared_mutex> lock(m_mtx);
        if (Node* f = buscar(key)) return f->m_data.m_value;
        m_tree.insert(Pair{key});
        return buscar(key)->m_data.m_value;
    }

    const Value& at(const Key& key) const {
        shared_lock<shared_mutex> lock(m_mtx);
        Node* f = buscar(key);
        if (!f) throw out_of_range("HashTable::at — key no existe");
        return f->m_data.m_value;
    }

    bool remove(const Key& key) {
        unique_lock<shared_mutex> lock(m_mtx);
        if (!buscar(key)) return false;
        m_tree.remove(Pair{key});
        return true;
    }

    bool   contains(const Key& k) const { shared_lock<shared_mutex> lock(m_mtx); return buscar(k) != nullptr; }
    size_t size()    const { return m_tree.size(); }
    bool   isEmpty() const { return m_tree.size() == 0; }

    // Iteradores: delegan en el arbol -> for (const auto& [k,v] : table)
    auto begin()       { return m_tree.begin(); }
    auto end()         { return m_tree.end();   }
    auto begin() const { return m_tree.begin(); }
    auto end()   const { return m_tree.end();   }

    friend ostream& operator<<(ostream& os, const HashTable& t) {
        shared_lock<shared_mutex> lock(t.m_mtx);
        os << "{";
        bool first = true;
        for (const Pair& p : t.m_tree.snapshot()) {
            if (!first) os << ", ";
            os << p;
            first = false;
        }
        return os << "}";
    }

    friend istream& operator>>(istream& is, HashTable& t) {
        char ch;
        if (!(is >> ch) || ch != '{') { is.setstate(ios::failbit); return is; }
        while ((is >> ws).peek() != '}') {
            Pair p;
            if (!(is >> p)) break;
            t[p.m_key] = p.m_value;
            if ((is >> ws).peek() == ',') is.get();
        }
        is.get();
        return is;
    }
};

// Alias de conveniencia: HashTableOf<Key, Value> == HashTable<HashTableTrait<Key,Value>>
template<typename K, typename V, typename C = less<K>>
using HashTableOf = HashTable<HashTableTrait<K, V, C>>;

void HashTableDemo();

#endif // __HASHTABLE_H__
