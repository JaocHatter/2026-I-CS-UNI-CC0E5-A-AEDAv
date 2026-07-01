// btree.h

#ifndef BTREE_H
#define BTREE_H

#include <iostream>
#include <sstream>
#include <mutex>
#include <shared_mutex> 
#include "util.h"
#include "BTreePage.h"

#define DEFAULT_BTREE_ORDER 3

template <typename TreeType>
class btree_iterator_base
{
public:
       using ObjectInfo = typename TreeType::ObjectInfo;
       using BTNode     = typename TreeType::BTNode;
protected:
       vector<pair<BTNode*, int>> m_Stack;
public:
       btree_iterator_base() = default;

       ObjectInfo &operator*()  { return m_Stack.back().first->m_Keys[m_Stack.back().second]; }
       ObjectInfo *operator->() { return &(**this); }

       friend bool operator==(const btree_iterator_base &a, const btree_iterator_base &b) {
              if( a.m_Stack.empty() && b.m_Stack.empty() ) return true;
              if( a.m_Stack.empty() || b.m_Stack.empty() ) return false;
              return a.m_Stack.back() == b.m_Stack.back();
       }
};

template <typename TreeType, bool IsForward>
class btree_iterator : public btree_iterator_base<TreeType>
{
       using BTNode = typename TreeType::BTNode;

       // Baja hasta el primer elemento en orden (forward) o el último (backward)
       void Descend(BTNode *p)
       {
              while( p && p->m_KeyCount > 0 )
              {
                     int i = IsForward ? 0 : p->m_KeyCount - 1;
                     this->m_Stack.push_back({p, i});
                     p = IsForward ? p->m_SubPages[0] : p->m_SubPages[p->m_KeyCount];
              }
       }
public:
       btree_iterator() = default;
       btree_iterator(BTNode *root, bool atEnd)
       {
              if( !atEnd && root && root->m_KeyCount > 0 )
                     Descend(root);
       }

       btree_iterator &operator++()
       {
              if( this->m_Stack.empty() )
                     return *this;

              BTNode *page = this->m_Stack.back().first;
              int     i    = this->m_Stack.back().second;

              BTNode *child = IsForward ? page->m_SubPages[i+1] : page->m_SubPages[i];
              this->m_Stack.back().second = IsForward ? i+1 : i-1;

              if( child )
              {
                     Descend(child);
              }
              else
              {
                     // Desapila ancestros ya agotados en la dirección de recorrido
                     while( !this->m_Stack.empty() &&
                            (IsForward ? this->m_Stack.back().second >= this->m_Stack.back().first->m_KeyCount
                                       : this->m_Stack.back().second <  0) )
                            this->m_Stack.pop_back();
              }
              return *this;
       }
};

template <typename TreeType>
using btree_forward_iterator  = btree_iterator<TreeType, true>;
template <typename TreeType>
using btree_backward_iterator = btree_iterator<TreeType, false>;

// Aqui remplazamos por un Trait
//template <typename keyType, typename ObjIDType = long>
template <typename Trait>
class BTree 
// this is the full version of the BTree
{
public:
       using keyType = typename Trait::keyType;
       using ObjIDType = typename Trait::ObjIDType;
       using ObjectInfo = typename Trait::Entry;
       using BTNode  = CBTreePage<Trait>;

       using forward_iterator  = btree_forward_iterator <BTree<Trait>>;
       using backward_iterator = btree_backward_iterator<BTree<Trait>>;

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

       template <typename Func, typename... Args>
       void   ReverseForEach(Func func, Args&&... args);

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

       forward_iterator  begin();
       forward_iterator  end();       
       backward_iterator rbegin();    
       backward_iterator rend();

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

template <typename Trait>
template <typename Func, typename... Args>
void   BTree<Trait>::ReverseForEach(Func func, Args&&... args) {
              unique_lock<shared_mutex> lock(m_mtx);
              ::ForEach(rbegin(), rend(), func, std::forward<Args>(args)...);
}

template <typename Trait>
typename BTree<Trait>::forward_iterator BTree<Trait>::begin()
{
       return forward_iterator(&m_Root, false);
}

template <typename Trait>
typename BTree<Trait>::forward_iterator BTree<Trait>::end()
{
       return forward_iterator(&m_Root, true);
}

template <typename Trait>
typename BTree<Trait>::backward_iterator BTree<Trait>::rbegin()
{
       return backward_iterator(&m_Root, false);
}

template <typename Trait>
typename BTree<Trait>::backward_iterator BTree<Trait>::rend()
{
       return backward_iterator(&m_Root, true);
}

#endif