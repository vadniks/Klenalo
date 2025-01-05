
#pragma once

#if __STDC_VERSION__ < 202311L /*C23*/ || !defined(__clang__) /*extensions*/ || !defined(__GNUC__) || !defined(__linux__) || !defined(__x86_64__) || !__LITTLE_ENDIAN__
#   error
#endif

#define TESTING true

typedef unsigned char byte;

#define staticAssert(x) static_assert(x)
#define atomic _Atomic
#define fallthrough [[fallthrough]];
#define nullable // everything that isn't market with nullable is considered to be not null

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)
#define boolToStr(x) (x ? "true" : "false")
#define xAlloca(x) (void*) ((byte[x]) {0})
#define unusedVariableBuffer(x) (x[1]) {0}
#define xalloc(x) SDL_calloc(x, 1)
#define USED(x) ((void) x)
#define STUB USED(0)
#define swapValues(x, y) {x ^= y; y ^= x; x ^= y;}

staticAssert(sizeof(char) == 1 & sizeof(short) == 2 & sizeof(int) == 4 & sizeof(long) == 8 & sizeof(void*) == 8);

inline void assert(bool const condition) { if (!condition) asm volatile ("call abort"); }
