#include <time.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "BTree.h"
#include "traits.h"
#include <chrono>
#include <thread>
#include <random>
#include "../types.h"

void BTreeVolumeDemo() {
    BTree<AscendingBTreeTrait<T3>> bt(64);
    vector<T3> allKeys;
    
    cout << "Manejo de Fechas en Gran Volumen" << endl;
    {    
        T3 n = 0;
        clock_t t0 = clock();

        for(int year = 100; year <= 2026; year++){
            for(int month = 1 ; month <= 12; month++){
                for(int day = 1; day <= 28; day++){
                    T3 key = (T3)(year * 10000L + month * 100L + day);
                    bt.Insert(key, key);
                    allKeys.push_back(key);
                    n++;
                }
            }
        }

        double seg = double(clock()-t0)/CLOCKS_PER_SEC;
        cout << "insertadas=" << n
         << " size=" << bt.size()
         << " height=" << bt.height()
         << " tiempo=" << seg << "s\n";
    }
}