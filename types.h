#ifndef __TYPES_H__
#define __TYPES_H__

// C/C++
// typedef int Type;

// C++11, C++14, C++17, C++20, C++23 ...
using Type = int;

// T1 must be int for 32-bit architecture and long long for 64-bit architecture
// It must work for windows, linux, iOS, macOS, android, etc.

using T1 = int;
using T2 = char; // será usado para el BTreeDemo
using T3 = long;

using Ref = long;

#endif // __TYPES_H__
