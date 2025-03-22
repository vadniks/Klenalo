
#pragma once

#if __STDC_VERSION__ < 202311L /*C23*/ || !defined(__clang__) /*extensions*/ || !defined(__GNUC__) || !defined(__linux__) || !defined(__x86_64__) || !__LITTLE_ENDIAN__ || (defined(__WORDSIZE) && __WORDSIZE != 64 || false)
#   error
#endif

typedef unsigned char byte;

// everything that isn't marked with nullable is considered to be not null
#ifdef __clang__
#pragma clang diagnostic ignored "-Wnullability-extension"
#pragma clang diagnostic ignored "-Wnullability-completeness"
#define nullable _Nullable
#else
#define nullable
#endif

#define staticAssert(x) static_assert(x)
#define atomic _Atomic
#define fallthrough [[fallthrough]];
#define packed [[gnu::packed]]
#define arraySize(x) (sizeof(x) / sizeof x[0])
#define inRange(x, c, y) c >= x && c <= y
#define unconst(x) *((typeof_unqual(x)*) &(x)) // TODO: replace with this in the all suitable places
#define cleanup(x) [[gnu::cleanup(x)]]
#define concatActual(x, y) x ## y
#define concat(x, y) concatActual(x, y) // yeah, that's weird, but it doesn't work directly

#ifdef __clang__
#include <Block.h> // https://fdiv.net/2015/10/08/emulating-defer-c-clang-or-gccblocks
void _deferHandler(void (^ const* const block)(void));
#define defer cleanup(_deferHandler) void (^ const concat(_defer_, __LINE__))(void) = ^
#else
#define defer(x) cleanup(x) const byte concat(_defer_, __LINE__);
#define deferHandler(x) static void x([[maybe_unused]] void* const deferred)
#endif

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define boolToStr(x) ((x) ? "true" : "false")
#define xAlloca(x) (void*) (byte[x]) {0}
#define unusedVariableBuffer(x) (x[1]) {0}
#define USED(x) ((void) (x))
#define STUB USED(0)
#define swapValues(x, y) {x ^= y; y ^= x; x ^= y;}

staticAssert(sizeof(char) == 1 & sizeof(short) == 2 & sizeof(int) == 4 & sizeof(long) == 8 & sizeof(void*) == 8);

void assert(const bool condition);

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

#define debugArgs(x, ...) {int printf(const char* const, ...); printf(x "\n", __VA_ARGS__);}
#define debug(x) debugArgs("%s", x)
typedef enum {PRINT_MEMORY_MODE_DEC, PRINT_MEMORY_MODE_HEX, PRINT_MEMORY_MODE_HEX_STR, PRINT_MEMORY_MODE_STR, PRINT_MEMORY_MODE_TRY_STR_HEX_FALLBACK} PrintMemoryMode;
void printMemory(const void* const memory, const int size, const PrintMemoryMode mode);
