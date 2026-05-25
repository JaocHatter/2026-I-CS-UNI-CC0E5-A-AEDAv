#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include "heap.h"

using namespace std;

void HeapDemo() {
    cout << "\n=== MinHeap Demo ===" << endl;

    Heap<MinHeapTrait<int>> minH;
    for (int v : {5, 3, 7, 1, 4, 6, 8, 2}) minH.insert(v);

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

    Heap<MaxHeapTrait<int>> maxH;
    for (int v : {5, 3, 7, 1, 4, 6, 8, 2}) maxH.insert(v);

    cout << "MaxHeap (nivel): " << maxH << endl;
    cout << "peek (max):      " << maxH.peek().getData() << endl;

    cout << "extraer en orden: ";
    while (!maxH.isEmpty()) {
        cout << maxH.peek().getData() << " ";
        maxH.extract();
    }
    cout << endl;

    cout << "\n=== I/O Demo ===" << endl;

    Heap<MinHeapTrait<int>> h1;
    for (int v : {9, 4, 2, 7, 5}) h1.insert(v, v * 10);

    ofstream fout("MinHeap.txt");
    fout << h1;
    fout.close();
    cout << "Guardado: " << h1 << endl;

    Heap<MinHeapTrait<int>> h2;
    ifstream fin("MinHeap.txt");
    fin >> h2;
    fin.close();
    cout << "Cargado:  " << h2 << endl;
    cout << "peek tras carga: " << h2.peek().getData() << " (esperado 2)" << endl;

    cout << "\n=== Copy/Move Demo ===" << endl;

    Heap<MinHeapTrait<int>> h3;
    for (int v : {10, 3, 6}) h3.insert(v);
    Heap<MinHeapTrait<int>> h4(h3);
    cout << "Original: " << h3 << endl;
    cout << "Copia:    " << h4 << endl;

    Heap<MinHeapTrait<int>> h5(move(h3));
    cout << "Movido:   " << h5 << endl;

    cout << "\n=== Concurrency test (5 threads x 1000 inserts) ===" << endl;

    Heap<MinHeapTrait<int>> cH;
    mutex printMtx;
    auto worker = [&cH](int start) {
        for (int i = start; i < start + 1000; ++i)
            cH.insert(i);
    };
    vector<thread> threads;
    for (int i = 0; i < 5; ++i) threads.emplace_back(worker, i * 1000);
    for (auto& t : threads) t.join();
    cout << "size=" << cH.size() << " (expected 5000) -> "
         << (cH.size() == 5000 ? "EXITO" : "FALLO") << endl;
    cout << "peek (min global): " << cH.peek().getData() << " (esperado 0)" << endl;
}
