#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <thread>
#include "graph.h"
#include "../types.h"

using namespace std;
using namespace graph;

using DG = CGraph<>;

void PrintGraph(const DG& g)
{
    vector<size_t> ids;
    for(auto it = g.nodes_cbegin(); it != g.nodes_cend(); ++it)
        ids.push_back(it->first);
    
    // Ordenamos por id porque unordered_map no garantiza el orden de iteracion: sin esto la salida cambia entre ejecuciones aunque el grafo sea el mismo
    sort(ids.begin(), ids.end());

    cout << "  Nodos (" << g.node_count() << "): ";
    for(size_t id : ids)
        cout << id << "(" << g.find_node(id)->data() << ") ";
    cout << "\n";

    vector<size_t> aristas;
    for(auto it = g.edges_cbegin(); it != g.edges_cend(); ++it)
        aristas.push_back(it->first);
    sort(aristas.begin(), aristas.end());

    cout << "  Aristas (" << g.edge_count() << "): ";
    for(size_t id : aristas) {
        const auto* e = g.find_edge(id);
        cout << id << ":" << e->source() << "->" << e->target() << "(w=" << e->weight() << ") ";
    }
    cout << "\n";
}

void GraphDemo()
{
    // Grafo de ejemplo:   1 -> 2 -> 4 -> 5 -> 1   (hay un ciclo)
    //                     1 -> 3 -> 4
    //                     6                       (nodo suelto)
    cout << "=== Agregar nodos y aristas ===" << endl;

    DG g;
    g.add_node(1, 10);
    g.add_node(2, 20);
    g.add_node(3, 30);
    g.add_node(4, 40);
    g.add_node(5, 50);
    g.add_node(6, 60);

    g.add_edge(100, 1, 2, 5);
    g.add_edge(101, 1, 3, 2);
    g.add_edge(102, 2, 4, 4);
    g.add_edge(103, 3, 4, 8);
    g.add_edge(104, 4, 5, 3);
    g.add_edge(105, 5, 1, 7);

    PrintGraph(g);

    // La arista va en un solo sentido: 1->2 no implica 2->1
    // Si quiero la vuelta, es OTRA arista con su propio id
    g.add_edge(106, 2, 1, 9);
    cout << "Tras agregar la vuelta 2->1, aristas = " << g.edge_count() << " (eran 6)\n";


    cout << "\n=== Buscar ===" << endl;

    cout << "Nodo 3   -> data = " << g.find_node(3)->data() << "\n";
    cout << "Nodo 99  -> " << (g.find_node(99) ? "existe" : "no existe") << "\n";

    const auto* e = g.find_edge(102);
    cout << "Arista 102 -> " << e->source() << " -> " << e->target()
         << " con peso " << e->weight() << "\n";
    cout << "Arista 999 -> " << (g.find_edge(999) ? "existe" : "no existe") << "\n";


    cout << "\n=== Quitar una arista ===" << endl;

    cout << "remove_edge(106) = " << (g.remove_edge(106) ? "ok" : "no estaba") << "\n";
    cout << "remove_edge(999) = " << (g.remove_edge(999) ? "ok" : "no estaba") << "\n";
    PrintGraph(g);


    cout << "\n=== Quitar un nodo ===" << endl;

    // El nodo 4 tiene 2 aristas entrantes (2->4, 3->4) y 1 saliente (4->5).
    // Al borrarlo deben irse las TRES: no pueden quedar aristas apuntando a un nodo muerto.
    cout << "Borro el nodo 4 (tiene 2 entrantes y 1 saliente)\n";
    g.remove_node(4);
    PrintGraph(g);

    cout << "remove_node(99) = " << (g.remove_node(99) ? "ok" : "no estaba") << "\n";


    cout << "\n=== Errores ===" << endl;

    try {
        g.add_node(1, 999);              // el id 1 ya esta usado
    } catch(const invalid_argument& ex) {
        cout << "add_node repetido  -> " << ex.what() << "\n";
    }

    try {
        g.add_edge(200, 1, 77, 5);       // el nodo 77 no existe
    } catch(const out_of_range& ex) {
        cout << "add_edge sin nodo  -> " << ex.what() << "\n";
    }

    try {
        g.add_edge(100, 2, 3, 5);        // el id de arista 100 ya esta usado
    } catch(const invalid_argument& ex) {
        cout << "add_edge repetida  -> " << ex.what() << "\n";
    }

    cout << "El grafo no se toco: " << g.node_count() << " nodos, "
         << g.edge_count() << " aristas\n";


    cout << "\n=== Operadores << y >> ===" << endl;
    cout << "cout << g  -> " << g << "\n";

    // Lo guardo en un string y lo vuelvo a leer en otro grafo: debe quedar igual
    stringstream ss;
    ss << g;

    DG copia;
    ss >> copia;
    cout << "tras >>    -> " << copia << "\n";
    cout << (copia.node_count() == g.node_count() && copia.edge_count() == g.edge_count()
             ? "OK: el grafo se reconstruyo igual\n" : "FALLO\n");

    // Si el texto pide una arista hacia un nodo que no existe, add_edge lanza
    DG malo;
    stringstream texto("{V:[(1,10)];E:[(9,1,77,5)]}");   // el nodo 77 no esta
    try {
        texto >> malo;
    } catch(const out_of_range& ex) {
        cout << "texto invalido -> " << ex.what() << "\n";
    }


    cout << "\n=== Limpiar ===" << endl;

    g.clear();
    cout << "Tras clear(): " << g.node_count() << " nodos, " << g.edge_count()
         << " aristas, empty() = " << (g.empty() ? "si" : "no") << "\n";


    cout << "\n=== Concurrencia ===" << endl;

    // 5 hilos insertando a la vez, cada uno con su rango de ids.
    // Si el shared_mutex no funcionara, saldrian menos de 5000.
    DG h;
    auto insertar = [&h](T1 hilo) {
        for(T1 i = 0; i < 1000; i++)
            h.add_node(hilo * 1000 + i, i);
    };

    thread t1(insertar, 1), t2(insertar, 2), t3(insertar, 3),
           t4(insertar, 4), t5(insertar, 5);
    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();

    cout << "5 hilos x 1000 add_node -> " << h.node_count() << " nodos (esperado 5000)\n";
    cout << (h.node_count() == 5000 ? "OK: sin condiciones de carrera\n"
                                    : "FALLO: hubo corrupcion\n");

    cout << "\n=== FIN ===" << endl;
}
