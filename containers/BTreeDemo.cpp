//#include <iostream.h>
#include <time.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include "BTree.h"
#include "traits.h"
#include "../types.h"

//const char * keys="CDAMPIWNBKEHOLJYQZFXVRTSGU";
const T2 * keys1 = "D1XJ2xTg8zKL9AhijOPQcEowRSp0NbW567BUfCqrs4FdtYZakHIuvGV3eMylmn";
const T2 * keys2 = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
const T2 * keys3 = "DYZakHIUwxVJ203ejOP9Qc8AdtuEop1XvTRghSNbW567BfiCqrs4FGMyzKLlmn";

const int BTreeSize = 3;

void BTreeDemo()
{
    // ── 1. Ascendente ──────────────────────────────────────────
    cout << "=== Ascendente (AscendingBTreeTrait<T2>) ===" << endl;
    {
        BTree<AscendingBTreeTrait<T2>> bt(BTreeSize);
        for(int i = 0; keys1[i]; i++)
            bt.Insert(keys1[i], (Ref)(i*i));
        cout << bt;
        cout << "Claves: " << bt.size() << "  Altura: " << bt.height() << "\n";
    }

    // ── 2. Descendente ─────────────────────────────────────────
    cout << "\n=== Descendente (DescendingBTreeTrait<T2>) ===" << endl;
    {
        BTree<DescendingBTreeTrait<T2>> bt(BTreeSize);
        for(int i = 0; keys1[i]; i++)
            bt.Insert(keys1[i], (Ref)(i*i));
        cout << bt;
        cout << "Claves: " << bt.size() << "  Altura: " << bt.height() << "\n";
    }

    // ── 3. Search (keys2: todos los posibles) ──────────────────
    cout << "\n=== Search ===" << endl;
    {
        BTree<AscendingBTreeTrait<T2>> bt(BTreeSize);
        for(int i = 0; keys1[i]; i++)
            bt.Insert(keys1[i], (Ref)(i*i));

        for(int i = 0; keys2[i]; i++) {
            Ref ObjID = bt.Search(keys2[i]);
            if( ObjID != -1 )
                cout << "Encontrado " << keys2[i] << " ID = " << ObjID << "\n";
            else
                cout << "No encontrado: " << keys2[i] << "\n";
        }
    }

    // ── 4. Remove (keys3: orden distinto) ──────────────────────
    cout << "\n=== Remove ===" << endl;
    {
        BTree<AscendingBTreeTrait<T2>> bt(BTreeSize);
        for(int i = 0; keys1[i]; i++)
            bt.Insert(keys1[i], (Ref)(i*i));

        for(int i = 0; keys3[i]; i++) {
            cout << "Removing " << keys3[i] << " ";
            if( bt.Remove(keys3[i], -1) )
                cout << keys3[i] << " removido!\n";
            else
                cout << "No encontrado: " << keys3[i] << "\n";
        }
        cout << "Arbol tras removes:\n";
        cout << bt; //NO MUESTRA NADA DEBIDO A QUE KEY1 Y KEY3 TIENEN LOS MISMOS CARACTERES PERO EN DIFERENTE ORDEN
    }

    // ── 5. ForEach variadic: contar mayusculas ──────────────────
    cout << "\n=== ForEach: Contar Mayusculas ===" << endl;
    {
        BTree<AscendingBTreeTrait<T2>> bt(BTreeSize);
        for(int i = 0; keys1[i]; i++)
            bt.Insert(keys1[i], (Ref)(i*i));

        T1 count = 0;
        bt.ForEach(
            [](auto &info, T1 &cnt) {
                if(info.key >= 'A' && info.key <= 'Z') {
                    cout << info.key << " ";
                    cnt++;
                }
            },
            count);

        cout << "\nMayusculas: " << count << "\n";
    }

    // ── 6. FirstThat variadic: primera clave > 'M' ─────────────
    cout << "\n=== FirstThat pero usando ForEach ===" << endl;
    {
        BTree<AscendingBTreeTrait<T2>> bt(BTreeSize);
        for(int i = 0; keys1[i]; i++)
            bt.Insert(keys1[i], (Ref)(i*i));

        auto *result = bt.ForEach(
            [](auto &info, T2 umbral) -> bool {
                return info.key > umbral;
            },
            'M'
        );
        if(result)
            cout << "Primera clave > 'M': '" << result->key
                 << "'  ID=" << result->ObjID << "\n";
        else
            cout << "No encontrada.\n";
    }

    cout << "\n=== Insert mediante operador \'>>\' ===" << endl;
    {
        BTree<AscendingBTreeTrait<T2>> bt(BTreeSize);
        for(int i = 0; keys1[i]; i++){
            std::istringstream iss("(A,10)(B,20)(C,30)(D,40)");
            iss >> bt;
            cout << bt;

            cout << "Claves: " << bt.size() << "  Altura: " << bt.height() << "\n";
            cout << "Search('B') = " << bt.Search('B') << "\n";
        }

        cout << bt;
    }

    cout << "\n=== Iteradores forward / backward ===" << endl;
    {
        BTree<AscendingBTreeTrait<T2>> bt(BTreeSize);
        for(int i = 0; keys1[i]; i++)
            bt.Insert(keys1[i], (Ref)(i*i));

        cout << "Forward (ascendente): ";
        bt.ForEach([](auto& info){ cout << info.key << " "; });
        cout << "\n";

        cout << "Backward (descendente): ";
        bt.ReverseForEach([](auto& info){ cout << info.key << " "; });
        cout << "\n";
    }
}










/*const char * keys="CDAMPIWNBKEHOLJYQZFXVRTSGU";
const char * keys2="CDAMPIWNBKEHOLJYQZFXVRTSGU";
const int BTreeSize = 3;
main (int argc, char * argv)
{
       //__int64 li;
       BTree <__int64> bt (BTreeSize);
       for (register int i = 0; i < 1000000; i++)
       {
               //cout<<"Inserting "<<keys[i]<<endl;
               bt.Insert(i, i-1);
               //bt.Print(cout);
       }

       for (i = 0; i < 1000; i++)
       {
               __int64 key = 975000+(::rand()%50000);
               //cout << "Searching " << (long)key << " ";
               long ObjID = bt.Search(key);
               if( ObjID != -1 )
                       cout << "Achei " << (long)key << " ID = " << ObjID << endl;
               else
                       cout <<"  Nao achei!" << (long)key << endl;
       }
       cout.flush();

       return 1;
}*/



/*const int BTreeSize = 3;
main (int argc, char * argv)
{
       int result, i;
       BTree <LONGLONG> bt(BTreeSize);
       result = bt.Create ("ernesto3-string-btree-start.dat",ios::in|ios::out);
       if (!result) { cout<<"Please delete testbt.dat"<<endl;return 0; }
       srand( (unsigned)time( NULL ) );
       LARGE_INTEGER key;
       for (i = 0; i < 1000000; i++)
       {
               //cout<<"Inserting "<<keys[i]<<endl;
               char strTmp[50];
               key.LowPart = rand();
               key.HighPart = rand();
               std::string str(strTmp);
               result = bt.Insert(key.QuadPart, i);
               //bt.Print(cout);
               if( i % 100000 == 0 )
               {       cout << i << endl; cout.flush();        }
       }
       //cout << "Searching D " << bt.Search();
       //bt.Search(1,1);
       cout.flush();
       return 1;
}*/
