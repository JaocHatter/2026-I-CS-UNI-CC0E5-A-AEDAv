#ifndef __BTREE_H__
#define __BTREE_H__

#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <shared_mutex>
#include <mutex>
#include <utility>
#include "../types.h"
#include "traits.h"
#include "BTreePage.h"

// BTree<Trait>: un solo param de template en lugar de BTree<keyType,ObjIDType>.
// Trait (BTreeTrait) encapsula value_type, Comp y Order,
// eliminando params redundantes y permitiendo reutilizar perfiles de árbol predefinidos.
template <typename Trait>
class BTree {
public:
    using value_type            = typename Trait::value_type;
    using Comp                  = typename Trait::Comp;
    static constexpr Size Order = Trait::Order;

    using Page  = BTreePage<Trait>;
    using Entry = typename Page::Entry;
    using Node  = Page;

    // Clase Iterator: permite usar BTree en range-based for y algoritmos STL.
    // La versión de antes no tenía ningún iterador; solo era posible imprimir con Print().
    // Implementación: pila de pares (página, índice) para recorrido inorden sin recursión.
    class Iterator {
        std::vector<std::pair<Page*, Size>> m_stack;
        const BTree* m_owner = nullptr;

        // Desciende por el hijo izquierdo de cada nodo acumulando niveles en la pila.
        void pushPath(Page* page, Size idx) {
            while (page && page->m_keyCount > 0) {
                m_stack.push_back({page, idx});
                page = page->m_subPages[idx];
                idx = 0;
            }
        }

    public:
        Iterator() = default;
        explicit Iterator(Page* root, const BTree* owner) : m_owner(owner) { pushPath(root, 0); }

        // shared_lock: múltiples lectores concurrentes pueden desreferenciar sin bloquearse.
        Entry& operator*() const {
            std::shared_lock<std::shared_mutex> lock(m_owner->m_mtx);
            return m_stack.back().first->m_keys[m_stack.back().second];
        }

        // Avanza al siguiente elemento en inorden: sube en la pila y baja por el hijo derecho.
        Iterator& operator++() {
            std::shared_lock<std::shared_mutex> lock(m_owner->m_mtx);
            auto [page, idx] = m_stack.back();
            m_stack.pop_back();

            if (idx + 1 < page->m_keyCount)
                m_stack.push_back({page, idx + 1});

            Page* rightChild = page->m_subPages[idx + 1];
            if (rightChild) pushPath(rightChild, 0);

            return *this;
        }

        Flag operator==(const Iterator& o) const { return m_stack == o.m_stack; }
        Flag operator!=(const Iterator& o) const { return !(*this == o); }
    };

    // begin/end const y non-const: necesarios para range-for sobre BTree const.
    // shared_lock en begin() protege la lectura de m_pRoot en contexto concurrente.
    Iterator begin()       { std::shared_lock<std::shared_mutex> lock(m_mtx); return Iterator(m_pRoot, this); }
    Iterator end()         { return Iterator(); }
    Iterator begin() const { std::shared_lock<std::shared_mutex> lock(m_mtx); return Iterator(m_pRoot, this); }
    Iterator end()   const { return Iterator(); }

private:
    // m_pRoot es puntero en lugar de objeto valor (m_Root de antes).
    // El puntero es necesario para implementar deepCopy en el copy constructor
    // y std::exchange en el move constructor.
    Page*  m_pRoot;
    Level  m_height;
    Flag   m_unique;
    Size   m_numKeys;
    // shared_mutex: permite lecturas concurrentes (shared_lock) y escrituras
    // exclusivas (unique_lock). antes no tenía ninguna protección de concurrencia.
    mutable std::shared_mutex m_mtx;

    // deepCopy: copia recursiva de todo el árbol.
    // Necesario para el copy constructor; antes carecía de él porque m_Root
    // era un valor y se copiaba superficialmente (sin clonar los hijos en heap).
    Page* deepCopy(Page* src) const {
        if (!src) return nullptr;
        auto* dst = new Page(src->m_maxKeys, src->m_unique);
        dst->m_maxKeysForChilds = src->m_maxKeysForChilds;
        dst->m_keyCount = src->m_keyCount;
        dst->m_keys     = src->m_keys;
        for (Size i = 0; i <= src->m_keyCount; ++i)
            dst->m_subPages[i] = deepCopy(src->m_subPages[i]);
        return dst;
    }

public:
    explicit BTree(Flag unique = true)
        : m_pRoot(new Page(2 * Order + 1, unique)), m_height(1), m_unique(unique), m_numKeys(0) {
        m_pRoot->setMaxKeysForChilds(Order);
    }

    // Copy constructor: la versión de antes carecía de él.
    // shared_lock en la fuente permite copiar mientras otros hilos leen el árbol origen.
    BTree(const BTree& o) : m_pRoot(nullptr), m_height(1), m_unique(true), m_numKeys(0) {
        std::shared_lock<std::shared_mutex> lock(o.m_mtx);
        m_pRoot   = deepCopy(o.m_pRoot);
        m_height  = o.m_height;
        m_unique  = o.m_unique;
        m_numKeys = o.m_numKeys;
    }

    // Move constructor: transfiere la propiedad de m_pRoot sin copiar el árbol.
    // std::exchange deja el origen en estado válido (nullptr/0) para su destrucción segura.
    BTree(BTree&& o) noexcept : m_pRoot(nullptr), m_height(1), m_unique(true), m_numKeys(0) {
        std::unique_lock<std::shared_mutex> lock(o.m_mtx);
        m_pRoot   = std::exchange(o.m_pRoot, nullptr);
        m_height  = std::exchange(o.m_height, 0);
        m_unique  = o.m_unique;
        m_numKeys = std::exchange(o.m_numKeys, 0);
    }

    BTree& operator=(const BTree& o) {
        if (this != &o) {
            std::unique_lock<std::shared_mutex> lk(m_mtx);
            std::shared_lock<std::shared_mutex> lo(o.m_mtx);
            delete m_pRoot;
            m_pRoot   = deepCopy(o.m_pRoot);
            m_height  = o.m_height;
            m_unique  = o.m_unique;
            m_numKeys = o.m_numKeys;
        }
        return *this;
    }

    BTree& operator=(BTree&& o) noexcept {
        if (this != &o) {
            std::unique_lock<std::shared_mutex> lk(m_mtx), lo(o.m_mtx);
            delete m_pRoot;
            m_pRoot   = std::exchange(o.m_pRoot, nullptr);
            m_height  = std::exchange(o.m_height, 0);
            m_unique  = o.m_unique;
            m_numKeys = std::exchange(o.m_numKeys, 0);
        }
        return *this;
    }

    ~BTree() { delete m_pRoot; }

    // unique_lock en insert/remove: garantiza acceso exclusivo durante la modificación.
    Flag insert(const value_type& key, Ref ref) {
        std::unique_lock<std::shared_mutex> lock(m_mtx);
        auto error = m_pRoot->insert(key, ref);
        if (error == bt_ErrorCode::duplicate) return false;
        ++m_numKeys;
        if (error == bt_ErrorCode::overflow) { m_pRoot->splitRoot(); ++m_height; }
        return true;
    }

    // remove retorna tuple<value_type,Ref> y lanza excepción si no encuentra.
    // antes retornaba bool con la clave perdida; no había forma de obtener el valor
    // eliminado. El centinela -1 era inválido si ObjID era no numérico o si -1 era válido.
    std::tuple<value_type, Ref> remove(const value_type& key) {
        std::unique_lock<std::shared_mutex> lock(m_mtx);
        value_type outValue{}; Ref outRef{};
        auto error = m_pRoot->remove(key, outValue, outRef);
        if (error == bt_ErrorCode::notFound)
            throw std::runtime_error("BTree::remove - clave no encontrada");
        --m_numKeys;
        if (error == bt_ErrorCode::rootMerged) --m_height;
        return {outValue, outRef};
    }

    // search retorna tuple<value_type,Ref> y lanza excepción si no encuentra.
    // antes retornaba ObjIDType(-1) como centinela, lo cual es incorrecto cuando
    // ObjIDType no es numérico o cuando -1 es un ID legítimo.
    std::tuple<value_type, Ref> search(const value_type& key) const {
        std::shared_lock<std::shared_mutex> lock(m_mtx);
        value_type outValue{}; Ref outRef{};
        if (!m_pRoot->search(key, outValue, outRef))
            throw std::runtime_error("BTree::search - clave no encontrada");
        return {outValue, outRef};
    }

    Size  size()   const { std::shared_lock<std::shared_mutex> lock(m_mtx); return m_numKeys; }
    Level height() const { std::shared_lock<std::shared_mutex> lock(m_mtx); return m_height; }
    Size  order()  const { return Order; }

    // forEach / firstThat / forEachPage delegan al iterador de página con shared_lock.
    template <typename Func, typename... Args>
    void forEach(Func func, Args&&... args) {
        std::shared_lock<std::shared_mutex> lock(m_mtx);
        m_pRoot->forEach(0, func, std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    Entry* firstThat(Func func, Args&&... args) {
        std::shared_lock<std::shared_mutex> lock(m_mtx);
        return m_pRoot->firstThat(0, func, std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    void forEachPage(Func func, Args&&... args) {
        std::shared_lock<std::shared_mutex> lock(m_mtx);
        m_pRoot->forEachPage(0, func, std::forward<Args>(args)...);
    }

    // toString / operator<< / operator>>: serialización del árbol como texto.
    // antes solo tenía Print(ostream&) que imprimía pero no permitía reconstruir.
    // El formato "[( dato:ref ),...]" es legible y reversible con operator>>.
    std::string toString() const {
        std::ostringstream oss;
        oss << "[";
        Flag first = true;
        for (const auto& e : *this) {
            if (!first) oss << ",";
            oss << "(" << e << ")";
            first = false;
        }
        oss << "]";
        return oss.str();
    }

    friend std::ostream& operator<<(std::ostream& os, const BTree& t) {
        return os << t.toString();
    }

    // Lee el formato producido por operator<< y re-inserta cada entrada en el árbol.
    friend std::istream& operator>>(std::istream& is, BTree& t) {
        Token ch;
        if (!(is >> ch) || ch != '[') { is.clear(std::ios_base::failbit); return is; }
        Entry e; Token paren;
        while (is >> ch && ch != ']')
            if (ch == '(')
                if (is >> e >> paren)
                    t.insert(e.m_data, e.m_ref);
        return is;
    }
};

#endif // __BTREE_H__
