// btree.h

#ifndef BTREE_H
#define BTREE_H

#include <iostream>
#include <tuple>
#include <vector>
#include <utility>
#include <stdexcept>
#include <shared_mutex>
#include <mutex>
#include "../types.h"
#include "traits.h"
#include "BTreePage.h"

#define DEFAULT_BTREE_ORDER 3

// BTree<Trait>: la version original era BTree<keyType, ObjIDType>. Ahora recibe un
// solo parametro (Trait, ver BTreeTrait) que encapsula keyType, ObjIDType y Order.
template <typename Trait>
class BTree
// this is the full version of the BTree
{
       using keyType   = typename Trait::value_type;
       using ObjIDType = typename Trait::ObjIDType;
       typedef CBTreePage<Trait> BTNode;            // useful shorthand

public:
       typedef typename BTNode::ObjectInfo ObjectInfo;
       using Entry = ObjectInfo;   // alias usado por el Demo: BT::Entry

       // ----------------------------------------------------------------------
       // Iterator inorden (recorrido sin recursion mediante una pila de
       // (pagina, indice)). Se mantiene respecto del original, que no tenia
       // ningun iterador (solo Print). Habilita el range-based for sobre BTree.
       // ----------------------------------------------------------------------
       class Iterator {
              std::vector<std::pair<BTNode*, Size>> m_stack;
              const BTree* m_owner = nullptr;

              void pushPath(BTNode* page, Size idx) {
                     while (page && page->m_KeyCount > 0) {
                            m_stack.push_back({page, idx});
                            page = page->m_SubPages[idx];
                            idx = 0;
                     }
              }
       public:
              Iterator() = default;
              explicit Iterator(BTNode* root, const BTree* owner) : m_owner(owner) { pushPath(root, 0); }

              ObjectInfo& operator*() const {
                     std::shared_lock<std::shared_mutex> lock(m_owner->m_mtx);
                     return m_stack.back().first->m_Keys[m_stack.back().second];
              }
              Iterator& operator++() {
                     std::shared_lock<std::shared_mutex> lock(m_owner->m_mtx);
                     auto [page, idx] = m_stack.back();
                     m_stack.pop_back();
                     if (idx + 1 < (Size)page->m_KeyCount)
                            m_stack.push_back({page, idx + 1});
                     BTNode* rightChild = page->m_SubPages[idx + 1];
                     if (rightChild) pushPath(rightChild, 0);
                     return *this;
              }
              bool operator==(const Iterator& o) const { return m_stack == o.m_stack; }
              bool operator!=(const Iterator& o) const { return !(*this == o); }
       };

       // shared_lock en begin(): protege la lectura de m_pRoot en contexto concurrente.
       Iterator begin()       { std::shared_lock<std::shared_mutex> lock(m_mtx); return Iterator(m_pRoot, this); }
       Iterator end()         { return Iterator(); }
       Iterator begin() const { std::shared_lock<std::shared_mutex> lock(m_mtx); return Iterator(m_pRoot, this); }
       Iterator end()   const { return Iterator(); }

public:
       BTree(Size order = DEFAULT_BTREE_ORDER, bool unique = true);
       // Copy / Move: el original no los tenia (m_Root era un objeto valor y se
       // copiaba superficialmente). Ahora m_pRoot es puntero y se clona en profundidad.
       BTree(const BTree& o);
       BTree(BTree&& o) noexcept;
       BTree& operator=(const BTree& o);
       BTree& operator=(BTree&& o) noexcept;
       ~BTree();

       bool                            insert(const keyType &key, const ObjIDType ObjID);
       // search / remove devuelven (clave, ObjID) en un tuple y lanzan excepcion si
       // no existe. El original devolvia ObjIDType(-1) como centinela (invalido si el
       // ObjID no es numerico o si -1 es un ID legitimo).
       std::tuple<keyType, ObjIDType>  search(const keyType &key) const;
       std::tuple<keyType, ObjIDType>  remove(const keyType &key);

       Size            size()   const { std::shared_lock<std::shared_mutex> lock(m_mtx); return m_NumKeys; }
       Size            height() const { std::shared_lock<std::shared_mutex> lock(m_mtx); return m_Height; }
       Size            order()  const { return m_Order; }

       void            Print(std::ostream &os) const {
              std::shared_lock<std::shared_mutex> lock(m_mtx);
              if (m_pRoot) m_pRoot->Print(os);
       }

       // ForEach / FirstThat variadic: delegan en la pagina raiz bajo shared_lock.
       template <typename Func, typename... Args>
       void ForEach(Func func, Args&&... args) {
              std::shared_lock<std::shared_mutex> lock(m_mtx);
              if (m_pRoot) m_pRoot->ForEach(0, func, std::forward<Args>(args)...);
       }
       template <typename Func, typename... Args>
       ObjectInfo* FirstThat(Func func, Args&&... args) {
              std::shared_lock<std::shared_mutex> lock(m_mtx);
              return m_pRoot ? m_pRoot->FirstThat(0, func, std::forward<Args>(args)...) : nullptr;
       }

protected:
       BTNode *m_pRoot;   // puntero (antes objeto valor m_Root) para permitir deepCopy/move
       Size    m_Height;  // height of tree
       Size    m_NumKeys; // number of keys
       Size    m_Order;   // order of tree (runtime, like the original)
       bool    m_Unique;  // Accept the elements only once ?
       // shared_mutex: lecturas concurrentes (shared_lock) y escrituras exclusivas
       // (unique_lock). El original no tenia ninguna proteccion de concurrencia.
       mutable std::shared_mutex m_mtx;

       BTNode* deepCopy(BTNode* src) const;
};

template <typename Trait>
typename BTree<Trait>::BTNode* BTree<Trait>::deepCopy(BTNode* src) const
{
       if (!src) return nullptr;
       BTNode* dst = new BTNode(src->m_MaxKeys, src->m_Unique);
       dst->m_MaxKeysForChilds = src->m_MaxKeysForChilds;
       dst->m_KeyCount = src->m_KeyCount;
       dst->m_Keys     = src->m_Keys;
       for (int i = 0; i <= src->m_KeyCount; ++i)
              dst->m_SubPages[i] = deepCopy(src->m_SubPages[i]);
       return dst;
}

template <typename Trait>
BTree<Trait>::BTree(Size order, bool unique)
                               : m_pRoot(new BTNode(2 * order + 1, unique)),
                                 m_Height(1),
                                 m_NumKeys(0),
                                 m_Order(order),
                                 m_Unique(unique)
{
       m_pRoot->SetMaxKeysForChilds(order);
}

template <typename Trait>
BTree<Trait>::BTree(const BTree& o)
       : m_pRoot(nullptr), m_Height(1), m_NumKeys(0), m_Order(DEFAULT_BTREE_ORDER), m_Unique(true)
{
       std::shared_lock<std::shared_mutex> lock(o.m_mtx);
       m_pRoot   = deepCopy(o.m_pRoot);
       m_Height  = o.m_Height;
       m_NumKeys = o.m_NumKeys;
       m_Order   = o.m_Order;
       m_Unique  = o.m_Unique;
}

template <typename Trait>
BTree<Trait>::BTree(BTree&& o) noexcept
       : m_pRoot(nullptr), m_Height(1), m_NumKeys(0), m_Order(DEFAULT_BTREE_ORDER), m_Unique(true)
{
       std::unique_lock<std::shared_mutex> lock(o.m_mtx);
       m_pRoot   = std::exchange(o.m_pRoot, nullptr);
       m_Height  = std::exchange(o.m_Height, 0);
       m_NumKeys = std::exchange(o.m_NumKeys, 0);
       m_Order   = o.m_Order;
       m_Unique  = o.m_Unique;
}

template <typename Trait>
BTree<Trait>& BTree<Trait>::operator=(const BTree& o)
{
       if (this != &o) {
              std::unique_lock<std::shared_mutex> lk(m_mtx);
              std::shared_lock<std::shared_mutex> lo(o.m_mtx);
              delete m_pRoot;
              m_pRoot   = deepCopy(o.m_pRoot);
              m_Height  = o.m_Height;
              m_NumKeys = o.m_NumKeys;
              m_Order   = o.m_Order;
              m_Unique  = o.m_Unique;
       }
       return *this;
}

template <typename Trait>
BTree<Trait>& BTree<Trait>::operator=(BTree&& o) noexcept
{
       if (this != &o) {
              std::unique_lock<std::shared_mutex> lk(m_mtx), lo(o.m_mtx);
              delete m_pRoot;
              m_pRoot   = std::exchange(o.m_pRoot, nullptr);
              m_Height  = std::exchange(o.m_Height, 0);
              m_NumKeys = std::exchange(o.m_NumKeys, 0);
              m_Order   = o.m_Order;
              m_Unique  = o.m_Unique;
       }
       return *this;
}

template <typename Trait>
BTree<Trait>::~BTree()
{
       delete m_pRoot;
}

template <typename Trait>
bool BTree<Trait>::insert(const keyType &key, const ObjIDType ObjID)
{
       std::unique_lock<std::shared_mutex> lock(m_mtx);
       bt_ErrorCode error = m_pRoot->Insert(key, ObjID);
       if( error == bt_duplicate )
               return false;
       m_NumKeys++;
       if( error == bt_overflow )
       {
               m_pRoot->SplitRoot();
               m_Height++;
       }
       return true;
}

template <typename Trait>
std::tuple<typename BTree<Trait>::keyType, typename BTree<Trait>::ObjIDType>
BTree<Trait>::remove(const keyType &key)
{
       std::unique_lock<std::shared_mutex> lock(m_mtx);
       keyType   outKey{};
       ObjIDType outID{};
       bt_ErrorCode error = m_pRoot->Remove(key, outKey, outID);
       if( error == bt_nofound )
               throw std::runtime_error("BTree::remove - clave no encontrada");
       m_NumKeys--;
       if( error == bt_rootmerged )
               m_Height--;
       return { outKey, outID };
}

template <typename Trait>
std::tuple<typename BTree<Trait>::keyType, typename BTree<Trait>::ObjIDType>
BTree<Trait>::search(const keyType &key) const
{
       std::shared_lock<std::shared_mutex> lock(m_mtx);
       keyType   outKey{};
       ObjIDType outID{};
       if( !m_pRoot->Search(key, outKey, outID) )
               throw std::runtime_error("BTree::search - clave no encontrada");
       return { outKey, outID };
}

#endif
