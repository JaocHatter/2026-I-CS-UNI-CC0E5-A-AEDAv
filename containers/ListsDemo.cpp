#include "linkedlist.h"
#include <fstream>
#include <iostream>
#include <thread>

template <typename T> 
void Add(T &n, T value){ 
    n += value; 
}


void LinkedListDemo(){
    LinkedList<T1, DescendingLinkedListTrait<T1>> list;
    list.insert(1, 15);
    list.insert(2, 25);
    list.insert(3, 35);
    list.insert(4, 45);
    list.insert(5, 55);
    cout << list << endl;
}

void DemoConcurrentLinkedList() {
  LinkedList<DescendingLinkedListTrait<int>> list;
  list.insert(0, 6);
  list.insert(0, 55);
  list.insert(0, 85);
  list.insert(0, 95);
  list.insert(0, 96);
  cout << list << endl;
 
  auto worker = [&list](int thread_id) {
    for (int i = 0; i < 100000; i++)
      list.ForEach(Add<T1>, 1);
    cout << "Thread " << thread_id << " terminado\n";
  };

  thread t1(worker, 1);
  thread t2(worker, 2);
  thread t3(worker, 3);
  thread t4(worker, 4);
  thread t5(worker, 5);

  t1.join();
  t2.join();
  t3.join();
  t4.join();
  t5.join();
  
  cout << "Resultado (esperado 500000): " << list << endl;
}

void ListsDemo() {
  LinkedListDemo();
  DemoConcurrentLinkedList();
}