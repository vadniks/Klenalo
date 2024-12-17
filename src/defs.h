
#pragma once

#if __STDC_VERSION__ < 202311L /* C23 */ || !defined(__GNUC__) || BYTE_ORDER != LITTLE_ENDIAN || !defined(__linux__)
#   error
#endif

typedef unsigned char byte;

#define staticAssert(x) _Static_assert(x)
#define atomic _Atomic
#define fallthrough [[fallthrough]];

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)
#define boolToStr(x) (x ? "true" : "false")
#define xAlloca(x) (void*) ((byte[x]) {0})
#define USED(x) ((void) x)
#define STUB USED(0)

staticAssert(sizeof(byte) == 1 & sizeof(short) == 2 & sizeof(int) == 4 & sizeof(long) == 8 & sizeof(void*) == 8);

inline void assert(bool condition) { if (!condition) asm volatile ("call abort"); }

void* mallocZeroed(unsigned long size); // { return SDL_calloc(size, 1); }
asm(
    ".global mallocZeroed\n"
    "mallocZeroed:\n"
    "movl $1, %esi\n"
    "jmp SDL_calloc"
);
