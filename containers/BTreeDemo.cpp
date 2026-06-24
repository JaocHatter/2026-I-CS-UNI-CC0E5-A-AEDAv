#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <cctype>
#include "../types.h"
#include "BTree.h"
#include "traits.h"
using namespace std;

//const char * keys="CDAMPIWNBKEHOLJYQZFXVRTSGU";
const char * keys1 = "D1XJ2xTg8zKL9AhijOPQcEowRSp0NbW567BUfCqrs4FdtYZakHIuvGV3eMylmn";
const char * keys2 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const char * keys3 = "DYZakHIUwxVJ203ejOP9Qc8AdtuEop1XvTRghSNbW567BfiCqrs4FGMyzKLlmn";

using KeyType = char;
using Trait   = BTreeTrait<KeyType>;
using BT      = BTree<Trait>;

struct Insertador {
    BT& arbol;
    Ref  idHilo;
    void operator()() const {
        for (Size k = 0; k < 150; ++k)
            arbol.insert(KeyType('A' + ((idHilo * 11 + k) % 26)), idHilo);
    }
};

// Devuelve las claves del arbol en orden, en una sola linea.
// ForEach visita las claves en orden ascendente (subpagina izq, clave, subpagina der).
static string clavesEnOrden(BT& t) {
    string s;
    t.ForEach([](BT::Entry& e, int /*level*/, string& out) { out += e.key; }, s);
    return s;
}

void DemoBTree() {
    // --- insert ---
    cout << "\n[insert]\n";
    BT arbol(3);
    for (Size idx = 0; keys1[idx] != '\0'; ++idx)
        arbol.insert(keys1[idx], Ref(idx * 3 + 1));
    cout << "  size=" << arbol.size() << "  height=" << arbol.height() << "  order=" << arbol.order() << "\n";

    // --- search (API: tuple + excepcion) ---
    cout << "\n[search]\n";
    try {
        auto [clave, id] = arbol.search('Z');
        cout << "  search('Z') -> encontrado  key=" << clave << "  ObjID=" << id << "\n";
    } catch (const runtime_error& ex) {
        cout << "  search('Z') -> " << ex.what() << "\n";
    }
    try {
        arbol.search('@');
    } catch (const runtime_error& ex) {
        cout << "  search('@') -> " << ex.what() << "\n";
    }

    // --- ForEach variadic (lambda + argumento extra por referencia) ---
    cout << "\n[ForEach variadic - contar claves por nivel]\n";
    vector<int> porNivel(arbol.height() + 1, 0);
    arbol.ForEach([](BT::Entry& info, int level, vector<int>& acum) {
        if (level < (int)acum.size()) acum[level]++;
    }, porNivel);
    for (size_t lvl = 0; lvl < porNivel.size(); ++lvl)
        cout << "  nivel " << lvl << ": " << porNivel[lvl] << " claves\n";

    // --- FirstThat variadic (predicado + argumento extra) ---
    cout << "\n[FirstThat variadic - primera clave > umbral]\n";
    KeyType umbral = 'W';
    BT::Entry* hallado = arbol.FirstThat(
        [](BT::Entry& info, int /*level*/, KeyType u) { return info.key > u; },
        umbral);
    if (hallado) cout << "  primera clave > '" << umbral << "' = '" << hallado->key
                      << "'  (ObjID=" << hallado->ObjID << ")\n";
    else         cout << "  ninguna clave > '" << umbral << "'\n";

    // --- conteo con ForEach variadic ---
    cout << "\n[conteo de digitos con ForEach]\n";
    Size cantDigitos = 0;
    arbol.ForEach([](BT::Entry& info, int /*level*/, Size& cont) {
        if (isdigit((Byte)info.key)) ++cont;
    }, cantDigitos);
    cout << "  digitos en el arbol: " << cantDigitos << "\n";

    // --- busqueda con FirstThat variadic ---
    cout << "\n[busqueda de clave 'N' con FirstThat]\n";
    BT::Entry* encontrado = arbol.FirstThat(
        [](BT::Entry& info, int /*level*/, KeyType objetivo) { return info.key == objetivo; },
        KeyType('N'));
    cout << "  busqueda('N') -> " << (encontrado ? "encontrado" : "no encontrado");
    if (encontrado) cout << " ObjID=" << encontrado->ObjID;
    cout << "\n";

    // --- remove (API: tuple + excepcion) ---
    cout << "\n[remove]\n";
    Size tamAntes = arbol.size();
    auto [claveEliminada, idEliminado] = arbol.remove('P');
    cout << "  remove('P') -> key=" << claveEliminada << " ObjID=" << idEliminado
         << "  size antes=" << tamAntes << "  size despues=" << arbol.size() << "\n";

    // --- recorrido en orden (ForEach) ---
    cout << "\n[recorrido en orden con ForEach]\n";
    cout << "  claves en orden: " << clavesEnOrden(arbol) << "\n";

    // --- UseCounter ---
    cout << "\n[GetUseCounter() - contador de accesos]\n";
    arbol.search('Q'); arbol.search('Q');
    arbol.search('r');
    arbol.ForEach([](BT::Entry& info, int /*level*/) {
        if (info.key == 'Q' || info.key == 'r')
            cout << "  '" << info.key << "' UseCounter=" << info.GetUseCounter() << "\n";
    });

    // --- concurrencia ---
    cout << "\n[Concurrencia]\n";
    BT arbolConcurrente(3);
    const Size numHilos = 4;
    vector<thread> hilos;
    hilos.reserve(numHilos);
    for (Size h = 0; h < numHilos; ++h)
        hilos.emplace_back(Insertador{arbolConcurrente, Ref(h + 1)});
    for (auto& hilo : hilos) hilo.join();
    cout << "  inserciones lanzadas: " << (numHilos * 150) << "\n";
    cout << "  size final (sin corrupcion, <= 26 claves unicas): " << arbolConcurrente.size() << "\n";
}
