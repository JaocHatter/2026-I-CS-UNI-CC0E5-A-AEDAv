#include <iostream>
#include <sstream>
#include <thread>
#include <vector>
#include "../types.h"
#include "BTree.h"
#include "traits.h"
using namespace std;

// Tipo de clave usado en el demo; cambiarlo aquí afecta todo el demo.
using TypeBTree = char;
using Trait     = Tree34Trait<TypeBTree>;   // árbol 3-4: raíz hasta 7 claves, hijos hasta 3
using BT        = BTree<Trait>;

// Inserta 200 claves rotando por el alfabeto para probar concurrencia sin duplicados excesivos.
static void concurrencyWorker(BT& tree, Ref workerId) {
    for (Size i = 0; i < 200; ++i)
        tree.insert(TypeBTree('a' + ((workerId * 7 + i) % 26)), workerId);
}

// [CAMBIO] DemoBTree() en lugar de void main(...): se integra con el main.cpp del proyecto
// como el resto de demos (DemoVector, DemoHeap, etc.). La versión del profesor
// tenía void main() no estándar que solo probaba inserción y dejaba búsqueda/eliminación
// comentadas en portugués.
void DemoBTree() {
    cout << "\n             PRUEBAS BTREE             \n";

    // --- insert ---
    cout << "\n[insert]\n";
    BT bt;
    const string keys = "D1XJ2xTg8zKL9AhijOPQcEowRSp0NbW567BUfCqrs4FdtYZakHIuvGV3eMylmn";
    for (Size i = 0; i < keys.size(); ++i)
        bt.insert(keys[i], Ref(i * i));
    cout << "  size=" << bt.size() << "  height=" << bt.height() << "  order=" << bt.order() << "\n";

    // --- search ---
    // [CAMBIO] search lanza excepcion en vez de retornar -1: se prueba el camino exitoso
    // y el fallido con try/catch para verificar ambas ramas.
    cout << "\n[search]\n";
    try {
        auto [val, ref] = bt.search('Z');
        cout << "  search('Z') -> encontrado  valor=" << val << "  ref=" << ref << "\n";
    } catch (const runtime_error& e) {
        cout << "  search('Z') -> " << e.what() << "\n";
    }
    try {
        bt.search('!');
    } catch (const runtime_error& e) {
        cout << "  search('!') -> " << e.what() << "\n";
    }

    // --- forEach variadic ---
    // [CAMBIO] Lambda con captura por referencia en lugar de funcion global con void*.
    cout << "\n[forEach variadic]\n";
    Size letterCount = 0;
    bt.forEach([](BT::Entry& e, Level, Size& count) {
        if (isalpha((Byte)e.m_data)) ++count;
    }, letterCount);
    cout << "  letras en el arbol: " << letterCount << "\n";

    // --- firstThat variadic ---
    cout << "\n[firstThat variadic]\n";
    auto* entry = bt.firstThat([](BT::Entry& e, Level, TypeBTree target) {
        return e.m_data == target;
    }, TypeBTree('M'));
    cout << "  firstThat('M') -> " << (entry ? "encontrado" : "no encontrado");
    if (entry) cout << " ref=" << entry->m_ref;
    cout << "\n";

    // --- remove ---
    // [CAMBIO] remove retorna el valor y ref eliminados en un tuple: el demo puede
    // mostrar que se extrajo correctamente en lugar de solo un bool.
    cout << "\n[remove]\n";
    Size beforeRemove = bt.size();
    auto [removedVal, removedRef] = bt.remove('A');
    cout << "  remove('A') -> eliminado: valor=" << removedVal << " ref=" << removedRef
         << "  size antes=" << beforeRemove << "  size despues=" << bt.size() << "\n";

    // --- iterador begin/end ---
    // [CAMBIO] range-for habilitado por el nuevo Iterator; antes era imposible sin ForEach.
    cout << "\n[for (auto& e : bt) - iterador inorder]\n";
    string inorderKeys;
    for (auto& e : bt) inorderKeys += e.m_data;
    cout << "  claves en orden: " << inorderKeys << "\n";

    // --- useCount ---
    // Verifica que search() incrementa el contador de accesos por clave.
    cout << "\n[useCount() - contador de accesos por clave]\n";
    bt.search('B'); bt.search('B'); bt.search('B');
    bt.search('C');
    for (auto& e : bt)
        if (e.m_data == 'B' || e.m_data == 'C')
            cout << "  '" << e.m_data << "' useCount=" << e.useCount() << "\n";

    // --- operator<< / operator>> ---
    // [CAMBIO] Serialización: convierte el arbol a string y lo reconstruye.
    // El profesor solo tenia Print() sin capacidad de reconstruccion.
    cout << "\n[operator<< / operator>>]\n";
    ostringstream oss;
    oss << bt;
    cout << "  Serializado : " << oss.str() << "\n";
    BT bt2;
    istringstream iss(oss.str());
    iss >> bt2;
    cout << "  Deserializado: " << bt2 << "\n";

    // --- copy constructor ---
    // [CAMBIO] Copy constructor: realiza deepCopy del arbol completo; la modificacion
    // en la copia no afecta al original.
    cout << "\n[Copy constructor]\n";
    BT copia(bt);
    copia.insert('!', 999);
    cout << "  Original : size=" << bt.size()    << "  " << bt    << "\n";
    cout << "  Copia    : size=" << copia.size() << "  " << copia << "\n";

    // --- move constructor ---
    // [CAMBIO] Move constructor: transfiere la propiedad sin copiar; la fuente queda vacia.
    cout << "\n[Move constructor]\n";
    BT movida(move(copia));
    cout << "  Movida   : size=" << movida.size() << "  " << movida << "\n";
    cout << "  Fuente tras move: size=" << copia.size() << "\n";

    // --- concurrencia ---
    // [CAMBIO] Prueba de thread-safety con shared_mutex: multiples hilos insertan
    // simultaneamente; el arbol debe terminar sin corrupcion (size <= 26 claves unicas).
    cout << "\n[Concurrencia]\n";
    BT concurrentTree;
    const Size kThreads = 5, kInsertsPerThread = 200;
    vector<thread> threads;
    threads.reserve(kThreads);
    for (Size i = 0; i < kThreads; ++i)
        threads.emplace_back(concurrencyWorker, std::ref(concurrentTree), Ref(i + 1));
    for (auto& t : threads) t.join();
    cout << "  inserciones concurrentes lanzadas: " << (kThreads * kInsertsPerThread) << "\n";
    cout << "  size final (sin corrupcion, <= 26 claves unicas): " << concurrentTree.size() << "\n";

    cout << "\n             FIN BTREE             \n";
}
