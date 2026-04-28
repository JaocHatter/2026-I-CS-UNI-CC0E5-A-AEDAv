#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <sstream>

#include "../types.h"
#include "linkedlist.h"
#include "doublelinkedlist.h"
#include "circularlinkedlist.h"
#include "circulardoublelinkedlist.h"

using namespace std;

template <typename Container>
void DemoList(Container& list, string fileName){
    list.insert(28, 15);
    list.insert(17, 25);
    list.insert(8, 35);
    list.insert(4, 45);
    list.insert(35, 55);
    cout << list << endl;
    // Grabar la lista en un archivo
    ofstream os(fileName);
    os << list << endl;

    // Leer la lista desde un archivo
    ifstream is(fileName);
    is >> list;
    cout << list << endl;
}

void LinkedListDemo(){
    cout << "\n--- LinkedList Ascendente ---" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> list;
    DemoList(list, "AscLL.txt");
    cout << "\n--- LinkedList Descendente ---" << endl;
    LinkedList<DescendingLinkedListTrait<T1>> list2;
    DemoList(list2, "DescLL.txt");
}

void DoubleLinkedListDemo(){
    cout << "\n--- DoubleLinkedList Ascendente ---" << endl;
    DoubleLinkedList<AscendingDLLTrait<T1>> list;
    DemoList(list, "AscDLL.txt");
    cout << "\n--- DoubleLinkedList Descendente ---" << endl;
    DoubleLinkedList<DescendingDLLTrait<T1>> list2;
    DemoList(list2, "DescDLL.txt");
}

void CircularLinkedListDemo(){
    cout << "\n--- CircleLinkedList Ascendente ---" << endl;
    CircleLinkedList<AscendingCircleLinkedListTrait<T1>> list;
    DemoList(list, "AscCLL.txt");
    cout << "\n--- CircleLinkedList Descendente ---" << endl;
    CircleLinkedList<DescendingCircleLinkedListTrait<T1>> list2;
    DemoList(list2, "DescCLL.txt");
}

void CircularDoubleLinkedListDemo(){
    cout << "\n--- CircularDoubleLinkedList Ascendente ---" << endl;
    CircularDoubleLinkedList<AscendingCDLLTrait<T1>> list;
    DemoList(list, "AscCDLL.txt");
    cout << "\n--- CircularDoubleLinkedList Descendente ---" << endl;
    CircularDoubleLinkedList<DescendingCDLLTrait<T1>> list2;
    DemoList(list2, "DescCDLL.txt");
}

template <typename Container>
void TestConcurrenciaGen(const string& nombre) {
    cout << "\nTEST DE CONCURRENCIA - " << nombre << endl;
    Container list;

    auto worker = [&list](int thread_id) {
        for(int i = 0; i < 1000; i++) {
            list.push_front(i, thread_id);
        }
    };

    thread t1(worker, 1);
    thread t2(worker, 2);
    thread t3(worker, 3);
    thread t4(worker, 4);
    thread t5(worker, 5);

    t1.join(); t2.join(); t3.join(); t4.join(); t5.join();

    cout << "Se lanzaron 5 hilos insertando 1000 elementos simultaneamente." << endl;
    cout << "Tamano de la lista (Esperado 5000): " << list.size() << endl;
    if(list.size() == 5000) {
        cout << "ESTADO: EXITO - El shared_mutex previno condiciones de carrera perfectamente." << endl;
    } else {
        cout << "ESTADO: FALLO - Hubo corrupcion de memoria." << endl;
    }
}

void TestConcurrencia() {
    TestConcurrenciaGen<LinkedList<AscendingLinkedListTrait<T1>>>("LinkedList");
    TestConcurrenciaGen<DoubleLinkedList<AscendingDLLTrait<T1>>>("DoubleLinkedList");
    TestConcurrenciaGen<CircleLinkedList<AscendingCircleLinkedListTrait<T1>>>("CircleLinkedList");
    TestConcurrenciaGen<CircularDoubleLinkedList<AscendingCDLLTrait<T1>>>("CircularDoubleLinkedList");
}

void TestOperators() {
    cout << "\nTEST DE OPERADORES" << endl;
    LinkedList<AscendingLinkedListTrait<T1>> list;

    // 1. Probamos operator>> (Lectura)
    cout << "Simulando lectura desde formato: [(10, 100), (20, 200), (30, 300)]" << endl;
    stringstream simulador_input("[(10, 100), (20, 200), (30, 300)]");
    simulador_input >> list;

    // 2. Probamos operator<< (Escritura)
    cout << "Lista luego de la lectura (operator<<): " << list << endl;

    // 3. Probamos operator[] (Acceso seguro por indice)
    cout << "Accediendo al indice [0] (operator[]): Dato -> " << list[0] << endl;
    cout << "Accediendo al indice [2] (operator[]): Dato -> " << list[2] << endl;

    // Probamos la excepcion del operator[] (Descomentar para probar)
    // cout << "Probando fuera de rango: " << list[5] << endl; // Lanzara la excepcion
}

void ListsDemo(){
    LinkedListDemo();
    DoubleLinkedListDemo();
    CircularLinkedListDemo();
    CircularDoubleLinkedListDemo();
    TestConcurrencia();
    TestOperators();
    cout << "\n=== FIN DE LAS PRUEBAS ===" << endl;
}
