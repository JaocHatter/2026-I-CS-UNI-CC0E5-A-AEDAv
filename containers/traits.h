#ifndef __TRAITS_H__
#define __TRAITS_H__
#include <functional>
#include "../types.h"   // [CAMBIO] necesario para Size en BTreeTrait

template <typename _Node, typename _Comp>
struct BaseTrait {
    using Node       = _Node;
    using value_type = typename _Node::value_type;
    using Comp       = _Comp;
};

// [CAMBIO] std::less / std::greater calificados explícitamente: evitan ambigüedad
// cuando el header se incluye en unidades sin 'using namespace std'.
template <typename _Node>
struct AscendingTrait  : public BaseTrait<_Node, std::less<typename _Node::value_type>>{};
template <typename _Node>
struct DescendingTrait : public BaseTrait<_Node, std::greater<typename _Node::value_type>>{};

// [CAMBIO] BTreeTrait: encapsula en un solo param de template el tipo de clave,
// el orden y el comparador. Permite pasar el "perfil" completo del árbol a
// BTree<Trait> sin repetir <keyType, ObjIDType, Order, Comp> en cada instancia.
template <typename _Value, Size _Order, typename _Comp = std::less<_Value>>
struct BTreeTrait {
    using value_type            = _Value;
    using Comp                  = _Comp;
    // Order como constexpr: conocido en compile-time → el compilador puede optimizar
    // loops internos y rechazar configuraciones inválidas antes de ejecutar.
    static constexpr Size Order = _Order;
};

// [CAMBIO] Alias de conveniencia para los dos tipos más comunes de árbol B*.
template <typename _Value, typename _Comp = std::less<_Value>>
using Tree23Trait = BTreeTrait<_Value, 2, _Comp>;  // árbol 2-3 (orden 2)

template <typename _Value, typename _Comp = std::less<_Value>>
using Tree34Trait = BTreeTrait<_Value, 3, _Comp>;  // árbol 3-4 (orden 3, el más usado)

#endif // __TRAITS_H__
