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


template <typename Trait>
class BTree
{
       using keyType   = typename Trait::value_type;
       using ObjIDType = typename Trait::ObjIDType;
       typedef CBTreePage<Trait> BTNode;            // useful shorthand

public:
       typedef typename BTNode::ObjectInfo ObjectInfo;
       using Entry = ObjectInfo;   // alias usado por el Demo: BT::Entry

       BTree(Size order = DEFAULT_BTREE_ORDER, bool unique = true);

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
              const_cast<BTNode&>(m_Root).Print(os);
       }

       template <typename Func, typename... Args>
       void ForEach(Func func, Args&&... args) {
              std::shared_lock<std::shared_mutex> lock(m_mtx);
              m_Root.ForEach(0, func, std::forward<Args>(args)...);
       }
       template <typename Func, typename... Args>
       ObjectInfo* FirstThat(Func func, Args&&... args) {
              std::shared_lock<std::shared_mutex> lock(m_mtx);
              return m_Root.FirstThat(0, func, std::forward<Args>(args)...);
       }

protected:
       BTNode  m_Root;    // raiz en el objeto (igual que el original del profesor)
       Size    m_Height;  // height of tree
       Size    m_NumKeys; // number of keys
       Size    m_Order;   // order of tree (runtime)
       bool    m_Unique;  // Accept the elements only once ?
       mutable std::shared_mutex m_mtx;
};

template <typename Trait>
BTree<Trait>::BTree(Size order, bool unique)
                               : m_Root(2 * order + 1, unique),
                                 m_Height(1),
                                 m_NumKeys(0),
                                 m_Order(order),
                                 m_Unique(unique)
{
       m_Root.SetMaxKeysForChilds(order);
}

template <typename Trait>
bool BTree<Trait>::insert(const keyType &key, const ObjIDType ObjID)
{
       std::unique_lock<std::shared_mutex> lock(m_mtx);
       bt_ErrorCode error = m_Root.Insert(key, ObjID);
       if( error == bt_duplicate )
               return false;
       m_NumKeys++;
       if( error == bt_overflow )
       {
               m_Root.SplitRoot();
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
       bt_ErrorCode error = m_Root.Remove(key, outKey, outID);
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
       if( !const_cast<BTNode&>(m_Root).Search(key, outKey, outID) )
               throw std::runtime_error("BTree::search - clave no encontrada");
       return { outKey, outID };
}

#endif
