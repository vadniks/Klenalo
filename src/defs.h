
#pragma once

#define DEBUG

#if __STDC_VERSION__ < 202311L /*C23*/ || !defined(__clang__) /*extensions*/ || !defined(__GNUC__) || !defined(__linux__) || !defined(__x86_64__) || !__LITTLE_ENDIAN__ || (defined(__WORDSIZE) && __WORDSIZE != 64 || false)
#   error
#endif

typedef unsigned char byte;

// everything that isn't marked with nullable is considered to be not null
#ifdef __clang__
#define nullable _Nullable
#else
#define nullable
#endif

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define boolToStr(x) ((x) ? "true" : "false")
#define xalloca(x) (void*) (byte[x]) {0}
#define xalloca2(x) __builtin_alloca(x)
#define unusedVariableBuffer(x) (x[1]) {0}
#define USED(x) ((void) (x))
#define STUB USED(0)
#define swapValues(x, y) {x ^= y; y ^= x; x ^= y;}
#define staticAssert(x) static_assert(x)
#define atomic _Atomic
#define fallthrough [[fallthrough]];
#define packed [[gnu::packed]]
#define arraySize(x) (sizeof(x) / sizeof x[0])
#define inRange(x, c, y) c >= x && c <= y
#define unconst(x) *((typeof_unqual(x)*) &(x))
#define overloadable [[clang::overloadable]]
#define offsetof(x, y) __builtin_offsetof(x, y)
#define cleanup(x) [[gnu::cleanup(x)]]
#define concatActual(x, y) x ## y
#define concat(x, y) concatActual(x, y) // yeah, that's weird, but it doesn't work directly
#define external [[gnu::visibility("default")]]

#if defined(__CLION_IDE__)
#define used [[gnu::used]] __attribute__((unused))
#else
#define used [[gnu::used]]
#endif

#define assignToStructWithConsts(x, ...) /* or just xmemcpy(x, &(typeof(*x)) {__VA_ARGS__}, sizeof *x); in case this is a mess */ { \
    struct packed _S {byte _[sizeof(typeof(*x))];}; \
    staticAssert(sizeof(struct _S) == sizeof(typeof(*x))); \
    *((struct _S*) x) = *(struct _S*) &(typeof(*x)) {__VA_ARGS__}; \
} // struct __ANY_STRUCT_TYPE_* const var = xmalloc(sizeof *var); if that struct contains const qualified fields; x is var (pointer to that struct)

#ifdef __clang__
#include <Block.h> // https://fdiv.net/2015/10/08/emulating-defer-c-clang-or-gccblocks
void _deferHandler(void (^ const* const block)(void));
#define defer cleanup(_deferHandler) void (^ const concat(_defer_, __LINE__))(void) = ^
#else
#define defer(x) cleanup(x) const byte concat(_defer_, __LINE__);
#define deferHandler(x) static void x([[maybe_unused]] void* const deferred)
#endif

typedef struct packed {byte _[];} VariableSizedStruct;

// TODO: add allocations tracker to dynamic allocator wrappers via hashtable (bypass wrappers or use arena on stack)

// TODO: add locks for each module's quit func

staticAssert(sizeof(char) == 1 & sizeof(short) == 2 & sizeof(int) == 4 & sizeof(long) == 8 & sizeof(void*) == 8);

external void assert(const bool condition);

// TODO: add a 'generic' copy/duplicate function

#ifdef __clang__
overloadable inline unsigned short swapBytes(short value) { return __builtin_bswap16(value); }
overloadable inline unsigned int swapBytes(int value) { return __builtin_bswap32(value); }
overloadable inline unsigned long swapBytes(long value) { return __builtin_bswap64(value); }
#else
#define swapBytes(x) _Generic((x), short: swapShort, int: swapInt, long: swapLong)(x)
inline unsigned short swapShort(short value) { return __builtin_bswap16(value); }
inline unsigned int swapInt(int value) { return __builtin_bswap32(value); }
inline unsigned long swapLong(long value) { return __builtin_bswap64(value); }
#endif

unsigned long xallocations(void);
void* nullable xmalloc(const unsigned long size);
void* nullable xcalloc(const unsigned long elements, const unsigned long size);
void* nullable xrealloc(void* nullable const pointer, const unsigned long size);
void xfree(void* nullable const memory);

typedef struct {
    void* nullable (* const malloc)(const unsigned long size);
    void* nullable (* const calloc)(const unsigned long elements, const unsigned long size);
    void* nullable (* const realloc)(void* nullable const pointer, const unsigned long size);
    void (* const free)(void* nullable const memory);
} Allocator;

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

inline unsigned long xstrnlen(const char* const string, const unsigned long maxSize) {
    unsigned long strnlen(const char* const, const unsigned long);
    return strnlen(string, maxSize);
}

#define debugArgs(x, ...) {int printf(const char* const, ...); printf(x "\n", __VA_ARGS__);}
#define debug(x) debugArgs("%s", x)
typedef enum {PRINT_MEMORY_MODE_DEC, PRINT_MEMORY_MODE_HEX, PRINT_MEMORY_MODE_HEX_STR, PRINT_MEMORY_MODE_STR, PRINT_MEMORY_MODE_TRY_STR_HEX_FALLBACK} PrintMemoryMode;
void printMemory(const void* const memory, const int size, const PrintMemoryMode mode);

// TODO: mock and unit test functions that uses 3rd party libraries by providing mock implementations of the libraries' functions simply defining those functions as they are weak symbols

void patchFunction(void* const original, void* const replacement); // overrides first 12 bytes of the original function with a trampoline to the replacement function

// TODO: add logger with various logging modes; add dynamic memory allocation tracker; render lvgl via opengl optimized textures via embedded support via lvgl's generic opengl driver
// TODO: make stack and heap use (double) linked list instead of growable array

// TODO: separate data structures into a standalone library and add red-black/(avl<--) tree (self-balancing binary search tree)
// TODO: separate crypto routines into a standalone library
// TODO: separate networking module into a standalone library
