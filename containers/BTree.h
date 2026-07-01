// btree.h

#ifndef BTREE_H
#define BTREE_H

#include <iostream>
#include <sstream>
#include <mutex>
#include <shared_mutex> 
#include "BTreePage.h"

#define DEFAULT_BTREE_ORDER 3

// Aqui remplazamos por un Trait
//template <typename keyType, typename ObjIDType = long>
template <typename Trait>
class BTree 
// this is the full version of the BTree
{
       typedef CBTreePage <Trait> BTNode;// alias para cbtreepage

public:
       using keyType = typename Trait::keyType;
       using ObjIDType = typename Trait::ObjIDType;
       using ObjectInfo = typename Trait::Entry;

       //typedef ObjectInfo iterator;
       /*
       typedef typename BTNode::lpfnForEach2    lpfnForEach2;
       typedef typename BTNode::lpfnForEach3    lpfnForEach3;
       typedef typename BTNode::lpfnFirstThat2  lpfnFirstThat2;
       typedef typename BTNode::lpfnFirstThat3  lpfnFirstThat3;
       typedef typename BTNode::ObjectInfo      ObjectInfo;
       */
protected:
       mutable shared_mutex m_mtx;

public:
       BTree(int order = DEFAULT_BTREE_ORDER, bool unique = true);
       ~BTree();
       //int           Open (char * name, int mode);
       //int           Create (char * name, int mode);
       //int           Close ();
       bool            Insert (const keyType key, const ObjIDType ObjID);
       bool            Remove (const keyType key, const ObjIDType ObjID);
       ObjIDType       Search (const keyType key);
       long            size()  { return m_NumKeys; }
       long            height() { return m_Height;      }
       long            GetOrder() { return m_Order;     }
       
       template <typename Func, typename... Args>
       decltype(auto)       ForEach( Func func, Args&&... args );

       friend ostream& operator<<(ostream& os, BTree& bt) {
              shared_lock<shared_mutex> lock(bt.m_mtx);
              bt.ForEach([&os](auto& info, int level){
                     for(int i = 0; i < level; i++){
                            os << "\t";  // indenta por profundidad
                     }
                     os << "(" << info.key << " , " << info.ObjID << ")\n";
              });
              return os;
       }

       friend istream& operator>>(istream& is, BTree& bt){
              unique_lock<shared_mutex> lock(bt.m_mtx);
              keyType key;
              ObjIDType id;
              char ch;

              while (is >> ch && ch == '('){
                     is >> key >> ch;
                     is >> id >> ch;
                     bt.Insert(key, id);
              }
              return is;
       }

protected:
       bool            m_Unique;  // Accept the elements only once ?
       int             m_Order;
       BTNode          m_Root;
       int             m_Height;   
       long            m_NumKeys; // number of keys
};

const int MaxHeight = 5;
template <typename Trait>
BTree<Trait>::BTree(int order, bool unique)
                               : m_Unique(unique),
                                 m_Order(order),
                                 m_Root(2 * order  + 1, unique),
                                 m_NumKeys(0)
{
       m_Root.SetMaxKeysForChilds(order);
       m_Height = 1;
}

template <typename Trait>
BTree<Trait>::~BTree()
{
}

template <typename Trait>
bool BTree<Trait>::Insert(const keyType key, const ObjIDType ObjID)
{
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
bool BTree<Trait>::Remove (const keyType key, const ObjIDType ObjID)
{
       bt_ErrorCode error = m_Root.Remove(key, ObjID);
       if( error == bt_duplicate || error == bt_nofound )
               return false;
       m_NumKeys--;

       if( error == bt_rootmerged )
               m_Height--;
       return true;
}

template <typename Trait>
typename Trait::ObjIDType BTree<Trait>::Search (const keyType key)
{
       ObjIDType ObjID = -1;
       m_Root.Search(key, ObjID);
       return ObjID;
}


template <typename Trait>
template <typename Func, typename... Args>
decltype(auto) BTree<Trait>::ForEach(Func func, Args&&... args)
{
       return m_Root.ForEach(func, 0, args...);
}


#endif