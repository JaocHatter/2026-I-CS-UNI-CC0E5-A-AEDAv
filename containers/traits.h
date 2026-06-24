#ifndef __TRAITS_H__
#define __TRAITS_H__
#include <functional> // para less y greater
#include "../types.h" // necesario para Size / Ref en BTreeTrait

template <typename _Node, typename _Comp>
struct BaseTrait{
    using Node       = _Node;
    using value_type = typename _Node::value_type;
    using Comp       = _Comp;
};

template <typename _Node>
struct AscendingTrait : public BaseTrait<_Node, std::less<typename _Node::value_type>>{
};
template <typename _Node>
struct DescendingTrait : public BaseTrait<_Node, std::greater<typename _Node::value_type>>{
};

// BTreeTrait: encapsula en un solo parametro de template el "perfil" del BTree:
// el tipo de clave (value_type), el tipo de identificador del objeto (ObjIDType)
// y el orden (Order). Reemplaza los parametros sueltos <keyType, ObjIDType> mas el
// argumento de orden en tiempo de ejecucion que tenia el BTree original.
template <typename _Value, Size _Order, typename _ObjID = Ref>
struct BTreeTrait {
    using value_type            = _Value;
    using ObjIDType             = _ObjID;
    static constexpr Size Order = _Order;
};

#endif // __TRAITS_H__
