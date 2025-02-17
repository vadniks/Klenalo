
#pragma once

#if __STDC_VERSION__ < 202311L /*C23*/ || !defined(__clang__) /*extensions*/ || !defined(__GNUC__) || !defined(__linux__) || !defined(__x86_64__) || !__LITTLE_ENDIAN__ || (defined(__WORDSIZE) && __WORDSIZE != 64 || false)
#   error
#endif

typedef unsigned char byte;

#define staticAssert(x) static_assert(x)
#define atomic _Atomic
#define fallthrough [[fallthrough]];
#define nullable // everything that isn't marked with nullable is considered to be not null

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define boolToStr(x) ((x) ? "true" : "false")
#define xAlloca(x) (void*) ((byte[x]) {0})
#define unusedVariableBuffer(x) (x[1]) {0}
#define USED(x) ((void) (x))
#define STUB USED(0)
#define swapValues(x, y) {x ^= y; y ^= x; x ^= y;}

staticAssert(sizeof(char) == 1 & sizeof(short) == 2 & sizeof(int) == 4 & sizeof(long) == 8 & sizeof(void*) == 8);

inline void assert(const bool condition) {
    [[gnu::noreturn]] void abort(void);
    if (!condition) abort();
}

#define swapBytes(x) _Generic((x), unsigned short: swapShort, unsigned int: swapInt, unsigned long: swapLong) (x)
inline unsigned short swapShort(unsigned short value) { return __builtin_bswap16(value); }
inline unsigned int swapInt(unsigned int value) { return __builtin_bswap32(value); }
inline unsigned long swapLong(unsigned long value) { return __builtin_bswap64(value); }

inline void* xmalloc(const unsigned long size) {
    void* SDL_malloc(const unsigned long);
    return SDL_malloc(size);
}

#define zmalloc(x) xcalloc(x, 1)
inline void* xcalloc(const unsigned numberOfElements, const unsigned long size) {
    void* SDL_calloc(const unsigned long, const unsigned long);
    return SDL_calloc(numberOfElements, size);
}

inline void* xrealloc(void* nullable const memory, const unsigned long size) {
    assert(size);
    void* SDL_realloc(void* const, const unsigned long);
    return SDL_realloc(memory, size);
}

static inline void xfree(void* const memory) { // static inline so it can be pointed to as a callback too
    void SDL_free(void* const);
    SDL_free(memory);
}

inline void* xmemset(void* const destination, const int value, const unsigned long length) {
    void* SDL_memset(void* const, const int, const unsigned long);
    return SDL_memset(destination, value, length);
}

inline void* xmemcpy(void* const destination, const void* const source, const unsigned long length) {
    void* SDL_memcpy(void* const, const void* const, const unsigned long);
    return SDL_memcpy(destination, source, length);
}

inline void* xmemmove(void* const destination, const void* const source, const unsigned long length) {
    void* SDL_memmove(void* const, const void* const, const unsigned long);
    return SDL_memmove(destination, source, length);
}

inline int xmemcmp(const void* const source1, const void* const source2, const unsigned long length) {
    int SDL_memcmp(const void* const, const void* const, const unsigned long);
    return SDL_memcmp(source1, source2, length);
}

inline void xyield(void) {
    void thrd_yield(void);
    thrd_yield();
}

// TODO: add const marker to everything that is immutable
