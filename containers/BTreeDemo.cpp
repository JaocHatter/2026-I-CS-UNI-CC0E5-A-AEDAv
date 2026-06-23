#include <iostream>
#include <sstream>
#include <vector>
#include "../types.h"
#include "BTree.h"
#include "traits.h"
using namespace std;

//const char * keys="CDAMPIWNBKEHOLJYQZFXVRTSGU";
const char * keys1 = "D1XJ2xTg8zKL9AhijOPQcEowRSp0NbW567BUfCqrs4FdtYZakHIuvGV3eMylmn";
const char * keys2 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const char * keys3 = "DYZakHIUwxVJ203ejOP9Qc8AdtuEop1XvTRghSNbW567BfiCqrs4FGMyzKLlmn";

using KeyType = char;
using Trait   = Tree34Trait<KeyType>;
using BT      = BTree<Trait>;

void DemoBTree() {
    // --- insert ---
    cout << "\n[insert]\n";
    BT arbol;
    for (Size idx = 0; keys1[idx] != '\0'; ++idx)
        arbol.insert(keys1[idx], Ref(idx * 3 + 1));
    cout << "  size=" << arbol.size() << "  height=" << arbol.height() << "  order=" << arbol.order() << "\n";

    // --- search ---
    cout << "\n[search]\n";
    try {
        auto [valor, referencia] = arbol.search('Z');
        cout << "  search('Z') -> encontrado  valor=" << valor << "  ref=" << referencia << "\n";
    } catch (const runtime_error& ex) {
        cout << "  search('Z') -> " << ex.what() << "\n";
    }
    try {
        arbol.search('@');
    } catch (const runtime_error& ex) {
        cout << "  search('@') -> " << ex.what() << "\n";
    }

    // --- conteo con iterador ---
    cout << "\n[conteo de digitos con iterador]\n";
    Size cantDigitos = 0;
    for (auto& entrada : arbol)
        if (isdigit((Byte)entrada.m_data)) ++cantDigitos;
    cout << "  digitos en el arbol: " << cantDigitos << "\n";

    // --- busqueda con iterador ---
    cout << "\n[busqueda de clave 'N' con iterador]\n";
    BT::Entry* encontrado = nullptr;
    for (auto& entrada : arbol)
        if (entrada.m_data == 'N') { encontrado = &entrada; break; }
    cout << "  busqueda('N') -> " << (encontrado ? "encontrado" : "no encontrado");
    if (encontrado) cout << " ref=" << encontrado->m_ref;
    cout << "\n";

    // --- remove ---
    cout << "\n[remove]\n";
    Size tamAntes = arbol.size();
    auto [claveEliminada, refEliminada] = arbol.remove('P');
    cout << "  remove('P') -> valor=" << claveEliminada << " ref=" << refEliminada
         << "  size antes=" << tamAntes << "  size despues=" << arbol.size() << "\n";

    // --- iterador inorder ---
    cout << "\n[for (auto& e : arbol) - iterador inorder]\n";
    string secuenciaOrdenada;
    for (auto& entrada : arbol) secuenciaOrdenada += entrada.m_data;
    cout << "  claves en orden: " << secuenciaOrdenada << "\n";

    // --- useCount ---
    cout << "\n[useCount() - contador de accesos]\n";
    arbol.search('Q'); arbol.search('Q');
    arbol.search('r');
    for (auto& entrada : arbol)
        if (entrada.m_data == 'Q' || entrada.m_data == 'r')
            cout << "  '" << entrada.m_data << "' useCount=" << entrada.useCount() << "\n";

    // --- serialización ---
    cout << "\n[operator<< / operator>>]\n";
    ostringstream flujoSalida;
    flujoSalida << arbol;
    cout << "  Serializado : " << flujoSalida.str() << "\n";
    BT arbol2;
    istringstream flujoEntrada(flujoSalida.str());
    flujoEntrada >> arbol2;
    cout << "  Deserializado: " << arbol2 << "\n";

    // --- copy constructor ---
    cout << "\n[Copy constructor]\n";
    BT clon(arbol);
    clon.insert('$', 777);
    cout << "  Original : size=" << arbol.size() << "  " << arbol << "\n";
    cout << "  Clon     : size=" << clon.size()  << "  " << clon  << "\n";

    // --- move constructor ---
    cout << "\n[Move constructor]\n";
    BT trasladado(move(clon));
    cout << "  Trasladado      : size=" << trasladado.size() << "  " << trasladado << "\n";
    cout << "  Fuente tras move: size=" << clon.size() << "\n";
}
