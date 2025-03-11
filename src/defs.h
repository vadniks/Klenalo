
#pragma once

#if __STDC_VERSION__ < 202311L /*C23*/ || !defined(__clang__) /*extensions*/ || !defined(__GNUC__) || !defined(__linux__) || !defined(__x86_64__) || !__LITTLE_ENDIAN__ || (defined(__WORDSIZE) && __WORDSIZE != 64 || false)
#   error
#endif

typedef unsigned char byte;

#define staticAssert(x) static_assert(x)
#define atomic _Atomic
#define fallthrough [[fallthrough]];
#define packed [[gnu::packed]]

// everything that isn't marked with nullable is considered to be not null
#ifdef __clang__
#pragma clang diagnostic ignored "-Wnullability-extension"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#define nullable _Nullable
#else
#define nullable
#endif

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define boolToStr(x) ((x) ? "true" : "false")
#define xAlloca(x) xmemset((byte[x]) {0}, 0, x)
#define unusedVariableBuffer(x) (x[1]) {0}
#define USED(x) ((void) (x))
#define STUB USED(0)
#define swapValues(x, y) {x ^= y; y ^= x; x ^= y;}

staticAssert(sizeof(char) == 1 & sizeof(short) == 2 & sizeof(int) == 4 & sizeof(long) == 8 & sizeof(void*) == 8);

inline void assert(const bool condition) {
    [[gnu::noreturn]] void abort(void); // TODO: add stacktrace/backtrace printing
    if (!condition) abort();
}

// TODO: add a 'generic' copy/duplicate function

#ifdef __clang__
[[clang::overloadable]] inline unsigned short swapBytes(unsigned short value) { return __builtin_bswap16(value); }
[[clang::overloadable]] inline unsigned int swapBytes(unsigned int value) { return __builtin_bswap32(value); }
[[clang::overloadable]] inline unsigned long swapBytes(unsigned long value) { return __builtin_bswap64(value); }
#else
#define swapBytes(x) _Generic((x), unsigned short: swapShort, unsigned int: swapInt, unsigned long: swapLong) (x)
inline unsigned short swapShort(unsigned short value) { return __builtin_bswap16(value); }
inline unsigned int swapInt(unsigned int value) { return __builtin_bswap32(value); }
inline unsigned long swapLong(unsigned long value) { return __builtin_bswap64(value); }
#endif

unsigned long xallocations(void);
void* nullable xmalloc(const unsigned long size);
void* nullable xcalloc(const unsigned long elements, const unsigned long size);
void* nullable xrealloc(void* nullable const pointer, const unsigned long size);
void xfree(void* nullable const memory);

inline void* xmemset(void* const destination, const int value, const unsigned long length) {
    void* memset(void* const, const int, const unsigned long);
    return memset(destination, value, length);
}

inline void* xmemcpy(void* const destination, const void* const source, const unsigned long length) {
    void* memcpy(void* const, const void* const, const unsigned long);
    return memcpy(destination, source, length);
}

inline void* xmemmove(void* const destination, const void* const source, const unsigned long length) {
    void* memmove(void* const, const void* const, const unsigned long);
    return memmove(destination, source, length);
}

inline int xmemcmp(const void* const source1, const void* const source2, const unsigned long length) {
    int memcmp(const void* const, const void* const, const unsigned long);
    return memcmp(source1, source2, length);
}

inline void xyield(void) {
    void thrd_yield(void);
    thrd_yield();
}

int printf(const char* const, ...);

[[deprecated("debug only")]] inline void printMemory(void* const memory, const int size) {
    for (int i = 0; i < size; printf("%x", ((byte*) memory)[i++]));
    printf("\n");
}
