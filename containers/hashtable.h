#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <functional>
#include <utility>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <stack>
#include <cstddef>
#include <string>
#include "BinaryTreeAVL.h"
using namespace std;

// Entrada (clave,valor) almacenada en cada arbol AVL. Es el value_type del
// arbol; el orden lo decide EntryKeyLess (solo por la clave).
template<typename Key, typename Value>
struct KVEntry {
    Key   key;
    Value value;
};

// Comparador del AVL: ordena las entradas unicamente por su clave.
template<typename Key, typename Value>
struct EntryKeyLess {
    bool operator()(const KVEntry<Key, Value>& a, const KVEntry<Key, Value>& b) const {
        return a.key < b.key;
    }
};

// ---------------------------------------------------------------------------
// Bucket = arbol AVL especializado en pares (clave,valor).
//
// Hereda toda la maquinaria de balanceo de BinaryTreeAVL (rotaciones, alturas)
// y de BinaryTree (mutex propio, destruccion recursiva). Solo agrega las
// operaciones por clave que la tabla necesita: get_or_create / find / erase.
// Cada metodo bloquea el mutex DEL PROPIO arbol -> permite "lock striping":
// hilos que tocan buckets distintos no se bloquean entre si.
// ---------------------------------------------------------------------------
template<typename Key, typename Value>
class AVLBucket
    : public BinaryTreeAVL<BaseTrait<AVLNode<KVEntry<Key, Value>>, EntryKeyLess<Key, Value>>> {
public:
    using Entry = KVEntry<Key, Value>;
    using Trait = BaseTrait<AVLNode<Entry>, EntryKeyLess<Key, Value>>;
    using Base  = BinaryTreeAVL<Trait>;
    using Node  = AVLNode<Entry>;
    using Base::Base;

    // Raiz cruda para que el iterador de la tabla recorra el arbol inorden.
    // NO toma lock (igual que begin()/end() de la base): solo seguro en
    // recorridos sin escritura concurrente.
    Node* root() const { return this->m_pRoot; }

    // Busca la clave; si no existe la inserta (valor por defecto) manteniendo
    // el arbol balanceado. Devuelve referencia al valor. 'inserted' indica si
    // hubo alta (la tabla lo usa para su contador global).
    Value& get_or_create(const Key& key, bool& inserted) {
        unique_lock lock(this->m_mtx);
        if (Node* n = find_node(key)) { inserted = false; return n->m_data.value; }
        // internal_insert es el hook AVL NO bloqueante: rebalancea y ++m_size.
        // Lo llamamos directamente (no insert()) para no re-bloquear el mutex.
        this->internal_insert(this->m_pRoot, Entry{key, Value{}});
        inserted = true;
        return find_node(key)->m_data.value;
    }

    bool contains_key(const Key& key) const {
        shared_lock lock(this->m_mtx);
        return find_node(key) != nullptr;
    }

    bool find_value(const Key& key, Value& out) const {
        shared_lock lock(this->m_mtx);
        const Node* n = find_node(key);
        if (!n) return false;
        out = n->m_data.value;
        return true;
    }

    bool erase_key(const Key& key) {
        unique_lock lock(this->m_mtx);
        if (!find_node(key)) return false;
        this->m_pRoot = remove_node(this->m_pRoot, Entry{key, Value{}});
        return true;
    }

private:
    // Igualdad/descenso con el MISMO comparador y convencion de ramas que el
    // arbol base (branch = !m_comp(...)): asi find/remove recorren el AVL por el
    // lado correcto sin importar como ordene EntryKeyLess.
    bool key_eq(const Node* n, const Entry& probe) const {
        return !this->m_comp(n->m_data, probe) && !this->m_comp(probe, n->m_data);
    }

    Node* find_node(const Key& key) const {
        Entry probe{key, Value{}};
        Node* cur = this->m_pRoot;
        while (cur) {
            if (key_eq(cur, probe)) return cur;
            cur = cur->m_pChild[!this->m_comp(cur->m_data, probe)];
        }
        return nullptr;
    }

    // Borrado BST sin lock (lo invoca erase_key, que ya sostiene el unique_lock).
    Node* remove_node(Node* node, const Entry& probe) {
        if (!node) return nullptr;
        if (key_eq(node, probe)) {
            if (!node->m_pChild[0] || !node->m_pChild[1]) {
                Node* child = node->m_pChild[0] ? node->m_pChild[0] : node->m_pChild[1];
                delete node; --this->m_size; return child;
            }
            Node* succ = node->m_pChild[1];
            while (succ->m_pChild[0]) succ = succ->m_pChild[0];
            node->m_data = succ->m_data;
            node->m_pChild[1] = remove_node(node->m_pChild[1], succ->m_data);
            return node;
        }
        auto branch = !this->m_comp(node->m_data, probe);
        node->m_pChild[branch] = remove_node(node->m_pChild[branch], probe);
        return node;
    }
};

// ---------------------------------------------------------------------------
// Tabla hash con encadenamiento por arbol AVL (un AVLBucket por casillero).
//
// Concurrencia (lock striping):
//   * m_mtx (shared_mutex) protege la ESTRUCTURA del arreglo de buckets
//     (puntero y capacidad). Las operaciones por clave lo toman en modo
//     compartido -> muchos hilos a la vez.
//   * Cada AVLBucket protege SU contenido con su propio mutex.
//   * m_size es atomico (lo modifican hilos que sostienen solo el shared_lock).
// ---------------------------------------------------------------------------
template<typename Key, typename Value, typename Hash = hash<Key>>
class HashTable {
public:
    using key_type   = Key;
    using value_type = Value;
    using Bucket     = AVLBucket<Key, Value>;
    using Entry      = typename Bucket::Entry;
    using MySelf     = HashTable<Key, Value, Hash>;

private:
    static constexpr size_t DEFAULT_CAPACITY = 16;

    Bucket*              m_buckets;
    size_t               m_capacity;
    atomic<size_t>       m_size;
    Hash                 m_hash;
    mutable shared_mutex m_mtx;   // protege el arreglo de buckets (estructura)

    size_t bucket_idx(const Key& k) const {
        return m_hash(k) % m_capacity;
    }

public:
    // Iterador hacia adelante: recorre los buckets en orden y, dentro de cada
    // uno, el arbol AVL en inorden (claves ordenadas). Como los iteradores de
    // arbol de la libreria, NO toma locks: usalo en recorridos sin escritura
    // concurrente (para lecturas seguras concurrentes usa toString()).
    class forward_iterator {
    public:
        using Node = typename Bucket::Node;
    private:
        HashTable*   m_table;
        size_t       m_bucket;
        stack<Node*> m_stack;
        Node*        m_node;

        void push_left(Node* n) { while (n) { m_stack.push(n); n = n->m_pChild[0]; } }

        // Siguiente nodo inorden; si el bucket se agota, salta al siguiente.
        void advance() {
            while (true) {
                if (!m_stack.empty()) {
                    Node* n = m_stack.top(); m_stack.pop();
                    m_node = n;
                    push_left(n->m_pChild[1]);
                    return;
                }
                ++m_bucket;
                if (m_bucket >= m_table->m_capacity) { m_node = nullptr; return; }
                push_left(m_table->m_buckets[m_bucket].root());
            }
        }
    public:
        forward_iterator(HashTable* t, bool is_end)
            : m_table(t), m_bucket(0), m_node(nullptr) {
            if (is_end) { m_bucket = t->m_capacity; return; }
            if (t->m_capacity) push_left(t->m_buckets[0].root());
            advance();
        }

        pair<const Key&, Value&> operator*() const {
            return {m_node->m_data.key, m_node->m_data.value};
        }
        forward_iterator& operator++() { advance(); return *this; }
        bool operator==(const forward_iterator& o) const {
            return m_bucket == o.m_bucket && m_node == o.m_node;
        }
        bool operator!=(const forward_iterator& o) const { return !(*this == o); }
    };

    forward_iterator begin() { return forward_iterator(this, false); }
    forward_iterator end()   { return forward_iterator(this, true);  }

    HashTable(size_t capacity = DEFAULT_CAPACITY)
        : m_buckets(new Bucket[capacity]), m_capacity(capacity), m_size(0), m_hash() {}

    virtual ~HashTable() { delete[] m_buckets; }

    // Constructor copia: reinserta cada par en arboles AVL nuevos (reconstruye
    // el balanceo). Toma shared_lock de 'other' para una copia consistente.
    HashTable(const HashTable& other)
        : m_buckets(nullptr), m_capacity(0), m_size(0) {
        shared_lock lock(other.m_mtx);
        m_capacity = other.m_capacity;
        m_hash     = other.m_hash;
        m_buckets  = new Bucket[m_capacity];
        for (size_t i = 0; i < other.m_capacity; ++i)
            for (const Entry& e : other.m_buckets[i].snapshot())
                (*this)[e.key] = e.value;
    }

    // Move constructor: roba el arreglo y deja a 'other' en estado vacio valido.
    HashTable(HashTable&& other)
        : m_buckets(nullptr), m_capacity(0), m_size(0) {
        unique_lock lock(other.m_mtx);
        m_buckets  = exchange(other.m_buckets,  new Bucket[1]);
        m_capacity = exchange(other.m_capacity, 1);
        m_size.store(other.m_size.exchange(0));
        m_hash     = move(other.m_hash);
    }

    // m[clave] -> referencia al valor (lo crea si no existe).
    Value& operator[](const Key& key) {
        shared_lock lock(m_mtx);                 // estructura estable
        bool inserted = false;
        Value& ref = m_buckets[bucket_idx(key)].get_or_create(key, inserted);
        if (inserted) ++m_size;
        return ref;
    }

    bool contains(const Key& key) const {
        shared_lock lock(m_mtx);
        return m_buckets[bucket_idx(key)].contains_key(key);
    }

    bool remove(const Key& key) {
        shared_lock lock(m_mtx);
        if (m_buckets[bucket_idx(key)].erase_key(key)) { --m_size; return true; }
        return false;
    }

    size_t size()    const { return m_size.load(); }
    bool   isEmpty() const { return m_size.load() == 0; }

    string toString() const {
        shared_lock lock(m_mtx);
        ostringstream oss;
        oss << "{";
        bool first = true;
        for (size_t i = 0; i < m_capacity; ++i) {
            for (const Entry& e : m_buckets[i].snapshot()) {
                if (!first) oss << ",";
                oss << e.key << ":" << e.value;
                first = false;
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
