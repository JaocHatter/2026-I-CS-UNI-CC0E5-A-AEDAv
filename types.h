#ifndef __TYPES_H__
#define __TYPES_H__
#include <cstddef>  // [CAMBIO] necesario para std::size_t y std::ptrdiff_t

// C/C++
// typedef int Type;

// C++11, C++14, C++17, C++20, C++23 ...
using Type = int;

// T1 must be int for 32-bit architecture and long long for 64-bit architecture
// It must work for windows, linux, iOS, macOS, android, etc.

using T1 = int;

using Ref = long;

// [CAMBIO] Tipos de propósito general para todo el codebase.
// Definirlos aquí permite cambiar el tamaño de plataforma en un solo lugar.
using Size   = std::size_t;     // índices y conteos sin signo
using SIndex = std::ptrdiff_t;  // índice con signo (necesario en loops: i >= 0)
using Flag   = bool;            // alias semántico para booleanos de control
using Level  = int;             // profundidad en árboles, pasada a forEach/firstThat
using Token  = char;            // carácter de parseo en operator>>
using Byte   = unsigned char;   // byte sin signo para casts seguros de char

#endif // __TYPES_H__
