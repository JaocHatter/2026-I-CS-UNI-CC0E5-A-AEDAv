#ifndef __TRAITS_H__
#define __TRAITS_H__
#include <functional>
#include "../types.h"   //  necesario para Size en BTreeTrait

template <typename _Node, typename _Comp>
struct BaseTrait {
    using Node       = _Node;
    using value_type = typename _Node::value_type;
    using Comp       = _Comp;
};

//  std::less / std::greater calificados explícitamente: evitan ambigüedad
// cuando el header se incluye en unidades sin 'using namespace std'.
template <typename _Node>
struct AscendingTrait  : public BaseTrait<_Node, std::less<typename _Node::value_type>>{};
template <typename _Node>
struct DescendingTrait : public BaseTrait<_Node, std::greater<typename _Node::value_type>>{};

//  BTreeTrait: encapsula en un solo param de template el tipo de clave,
// el orden y el comparador. Permite pasar el "perfil" completo del árbol a
// BTree<Trait> sin repetir <keyType, ObjIDType, Order, Comp> en cada instancia.
template <typename _Value, Size _Order, typename _Comp = std::less<_Value>>
struct BTreeTrait {
    using value_type            = _Value;
    using Comp                  = _Comp;
    static constexpr Size Order = _Order;
};


#endif // __TRAITS_H__
