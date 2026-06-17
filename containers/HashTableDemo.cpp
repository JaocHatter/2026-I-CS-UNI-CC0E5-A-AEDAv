#include <iostream>
#include <fstream>
#include <thread>
#include <vector>

#include "hashtable.h"
#include "../types.h"
using namespace std;

void HashTableDemo() {
    cout << "\n=== HashTable Demo ===" << endl;

    HashTableOf<Type, Type> m;

    m[5] = 3;
    m[10] = 7;
    m[1]  = 42;
    m[20] = 99;
    m[5]  = 100;  // actualiza valor existente

    cout << "Tabla: " << m << endl;
    cout << "m[5]  = " << m[5]  << " (esperado 100)" << endl;
    cout << "m[10] = " << m[10] << " (esperado 7)"   << endl;
    cout << "size  = " << m.size() << " (esperado 4)" << endl;

    cout << "\n--- structured bindings ---" << endl;
    for (const auto& [key, value] : m)
        cout << key << " -> " << value << endl;

    cout << "\n--- contains / remove ---" << endl;
    cout << "contains(10): " << boolalpha << m.contains(10) << endl;
    m.remove(10);
    cout << "contains(10) tras remove: " << m.contains(10) << endl;
    cout << "size tras remove: " << m.size() << " (esperado 3)" << endl;

    cout << "\n=== I/O Demo ===" << endl;

    HashTableOf<Type, Type> h1;
    h1[1] = 11;
    h1[2] = 22;
    h1[3] = 33;

    ofstream fout("HashTable.txt");
    fout << h1;
    fout.close();
    cout << "Guardado: " << h1 << endl;

    HashTableOf<Type, Type> h2;
    ifstream fin("HashTable.txt");
    fin >> h2;
    fin.close();
    cout << "Cargado:  " << h2 << endl;
    cout << "h2[2] = " << h2[2] << " (esperado 22)" << endl;

    cout << "\n=== Copy/Move Demo ===" << endl;

    HashTableOf<Type, Type> h3;
    h3[7] = 70; h3[8] = 80;

    HashTableOf<Type, Type> h4(h3);
    cout << "Original: " << h3 << endl;
    cout << "Copia:    " << h4 << endl;

    HashTableOf<Type, Type> h5(move(h3));
    cout << "Movido:   " << h5 << endl;

    cout << "\n=== HashTableOf<Type2, Type> Demo ===" << endl;

    HashTableOf<Type2, Type> words;
    words["hola"]    = 1;
    words["mundo"]   = 2;
    words["aeda"]    = 3;
    words["hola"]   += 10;

    cout << "Tabla string: " << words << endl;
    for (const auto& [k, v] : words)
        cout << k << " -> " << v << endl;

    cout << "\n=== Concurrency test (5 threads x 1000 inserts) ===" << endl;

    HashTableOf<Type, Type> cH;
    auto worker = [&cH](Type start) {
        for (Type i = start; i < start + 1000; ++i)
            cH[i] = i * 2;
    };
    vector<thread> threads;
    for (T1 i = 0; i < 5; ++i) threads.emplace_back(worker, i * 1000);
    for (auto& t : threads) t.join();

    cout << "size=" << cH.size() << " (expected 5000) -> "
         << (cH.size() == 5000 ? "EXITO" : "FALLO") << endl;
    cout << "cH[999] = " << cH[999] << " (esperado 1998)" << endl;
}
