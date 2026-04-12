#include <cstddef>
#include <iostream>
#include <string>
#include <fstream>
#include "vector.h"

using namespace std;

void DemoVector(){
    Vector<T1> v1(10);
    v1.push_back(1);
    v1.push_back(2);
    v1.push_back(-1);
    v1.push_back(4);
    cout << v1.toString() << endl;
    cout << v1 << endl;
    // cout << "hola" << 5 << endl;
    // cout.operator<<("hola")
    // ==============
    //           cout << 5 << endl;
    //           =========
    //                cout << endl;

    Vector<string> v2(10);
    v2.push_back("Hola");
    v2.push_back("Mundo");
    v2.push_back("!");
    cout << v2 << endl;
    cout << v2.toString() << endl;

    //Showing the new vectors
    ifstream isfile("new_temp.txt");
    
    if(isfile.is_open()){
        cout << "File opened successfully" << endl;
        isfile >> v1;
        isfile >> v2;
        isfile.close();
    }else{
        cerr << "Could not open the file!" << endl;
    }

    cout << v1 << endl;
    cout << v2 << endl;

    ofstream of("temp.txt");
    of << v1 << endl;
    of << v2 << endl;
    of.close(); 
}
