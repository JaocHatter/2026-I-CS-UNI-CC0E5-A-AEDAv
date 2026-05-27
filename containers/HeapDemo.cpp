#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include "heap.h"
#include "../types.h"

using namespace std;

void HeapDemo() {
    cout << "\n=== MinHeap Demo ===" << endl;

    Heap<MinHeapTrait<Type>> minH;
    for (Type v : {5, 3, 7, 1, 4, 6, 8, 2}) minH.insert(v);

    cout << "MinHeap (nivel): " << minH << endl;
    cout << "peek (min):      " << minH.peek().getData() << endl;
    cout << "size:            " << minH.size() << endl;

    cout << "range-for: ";
    for (auto& v : minH) cout << v << " ";
    cout << endl;

    cout << "extraer en orden: ";
    while (!minH.isEmpty()) {
        cout << minH.peek().getData() << " ";
        minH.extract();
    }
    cout << endl;

    cout << "\n=== MaxHeap Demo ===" << endl;

    Heap<MaxHeapTrait<Type>> maxH;
    for (Type v : {5, 3, 7, 1, 4, 6, 8, 2}) maxH.insert(v);

    cout << "MaxHeap (nivel): " << maxH << endl;
    cout << "peek (max):      " << maxH.peek().getData() << endl;

    cout << "extraer en orden: ";
    while (!maxH.isEmpty()) {
        cout << maxH.peek().getData() << " ";
        maxH.extract();
    }
    cout << endl;

    cout << "\n=== I/O Demo ===" << endl;

    Heap<MinHeapTrait<Type>> h1;
    for (Type v : {9, 4, 2, 7, 5}) h1.insert(v, v * 10);

    ofstream fout("MinHeap.txt");
    fout << h1;
    fout.close();
    cout << "Guardado: " << h1 << endl;

    Heap<MinHeapTrait<Type>> h2;
    ifstream fin("MinHeap.txt");
    fin >> h2;
    fin.close();
    cout << "Cargado:  " << h2 << endl;
    cout << "peek tras carga: " << h2.peek().getData() << " (esperado 2)" << endl;

    cout << "\n=== Copy/Move Demo ===" << endl;

    Heap<MinHeapTrait<Type>> h3;
    for (Type v : {10, 3, 6}) h3.insert(v);
    Heap<MinHeapTrait<Type>> h4(h3);
    cout << "Original: " << h3 << endl;
    cout << "Copia:    " << h4 << endl;

    Heap<MinHeapTrait<Type>> h5(move(h3));
    cout << "Movido:   " << h5 << endl;

    cout << "\n=== Concurrency test (5 threads x 1000 inserts) ===" << endl;

    Heap<MinHeapTrait<Type>> cH;
    mutex printMtx;
    auto worker = [&cH](Type start) {
        for (Type i = start; i < start + 1000; ++i)
            cH.insert(i);
    };
    vector<thread> threads;
    for (T1 i = 0; i < 5; ++i) threads.emplace_back(worker, i * 1000);
    for (auto& t : threads) t.join();
    cout << "size=" << cH.size() << " (expected 5000) -> "
         << (cH.size() == 5000 ? "EXITO" : "FALLO") << endl;
    cout << "peek (min global): " << cH.peek().getData() << " (esperado 0)" << endl;
}
