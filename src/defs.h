
#pragma once

#define DEBUG
//#define TESTING

#if __STDC_VERSION__ < 202400L /*C2Y*/ || !defined(__clang__) /*extensions*/ || !defined(__GNUC__) /*glibc*/ || \
    !defined(__linux__) /*api*/ || !defined(_GNU_SOURCE) /*api*/ || !defined(__x86_64__) || !__LITTLE_ENDIAN__ || \
    (defined(__WORDSIZE) && __WORDSIZE != 64 || false)
#   error
#endif

typedef unsigned char byte;

// everything (function return types, function parameter type, struct field type) that isn't marked with nullable is considered to be non-null
#ifdef __clang__
#define nullable _Nullable
#else
#define nullable
#endif

// if argument of particular function is like ..., bool cond1OrCond2, ... then the logic would be to evaluate either the cond1 or cond2 in the fallowing fashion: cond1OrCond2=true -> cond1, cond1OrCond2=false -> cond2 and vice versa; for the use with if-else

#define min(x, y) ((x) < (y) ? (x) : (y))
#define max(x, y) ((x) > (y) ? (x) : (y))
#define boolToStr(x) ((x) ? "true" : "false")
#define xalloca(x) (void*) (byte[x]) {0}
#define xalloca2(x) __builtin_alloca(x)
#define unusedVariableBuffer(x) (x[1]) {0} // or just &(x) {0}
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
#define returnAddr __builtin_return_address(0)
#define cleanup(x) [[gnu::cleanup(x)]] // aka destructor, raii, deffer, auto freeing
#define concatActual(x, y) x ## y
#define concat(x, y) concatActual(x, y) // yeah, that's weird, but it doesn't work directly
#define stringifyActual(x) #x
#define stringify(x) stringifyActual(x)
#define export [[gnu::visibility("default")]]
#define noinline [[clang::noinline]]
#define xinline [[clang::always_inline]] inline

#if defined(__CLION_IDE__)
#define used __attribute__((unused))
#else
#define used [[gnu::used]]
#endif

// or just xmemcpy(x, &(typeof(*x)) {__VA_ARGS__}, sizeof *x); in case this is a mess
#define assignToStructWithConsts(x, ...) { \
    struct packed _S {byte _[sizeof(typeof(*x))];}; \
    staticAssert(sizeof(struct _S) == sizeof(typeof(*x))); \
    *((struct _S*) x) = *(struct _S*) &(typeof(*x)) {__VA_ARGS__}; \
} // struct __ANY_STRUCT_TYPE_* const var = xmalloc(sizeof *var); if that struct contains const qualified fields; x is var (pointer to that struct)

#ifdef __clang__
#include <Block.h> // https://fdiv.net/2015/10/08/emulating-defer-c-clang-or-gccblocks

void _deferHandler(void (^ const* const block)(void));
#define defer cleanup(_deferHandler) void (^ const concat(_defer_, __LINE__))(void) = ^

used typedef struct Block_literal_1 { // https://clang.llvm.org/docs/Block-ABI-Apple.html
    used void* isa;
    used int flags;
    used int reserved;
    void/*return type*/ (* used invoke)(struct Block_literal_1*/*, void - parameter types...*/);
    used struct Block_descriptor_1 {
        used unsigned long int reserved;
        used unsigned long int size;
        void (* used copy_helper)(void* dst, void* src);
        void (* used dispose_helper)(void* src);
        used const char* signature;
    }* descriptor;
} BlockLiteral;

#else
#define defer(x) cleanup(x) const byte concat(_defer_, __LINE__);
#define deferHandler(x) static void x([[maybe_unused]] void* const deferred)
#endif

typedef struct packed {byte _[];} VariableSizedStruct;

// TODO: add locks for each module's quit func

staticAssert(sizeof(char) == 1 & sizeof(short) == 2 & sizeof(int) == 4 & sizeof(long) == 8 & sizeof(void*) == 8);

void assert(const bool condition);

// TODO: add a 'generic' copy/duplicate function

#ifdef __clang__
overloadable inline unsigned short swapBytes(const short value) { return __builtin_bswap16(value); }
overloadable inline unsigned int swapBytes(const int value) { return __builtin_bswap32(value); }
overloadable inline unsigned long swapBytes(const long value) { return __builtin_bswap64(value); }
#else
#define swapBytes(x) _Generic((x), short: swapShort, int: swapInt, long: swapLong)(x)
inline unsigned short swapShort(const short value) { return __builtin_bswap16(value); }
inline unsigned int swapInt(const int value) { return __builtin_bswap32(value); }
inline unsigned long swapLong(const long value) { return __builtin_bswap64(value); }
#endif

void checkUnfreedAllocations(void); // should only be called once and at the end of main()
void* xmalloc(const unsigned long size);
void* xcalloc(const unsigned long elements, const unsigned long size);
void* nullable xrealloc(void* nullable const pointer, const unsigned long size); // returns null only when size is zero, thus acting as xfree
void xfree(void* nullable const memory);

typedef struct {
    void* (* malloc)(const unsigned long size);
    void* (* calloc)(const unsigned long elements, const unsigned long size);
    void* (* realloc)(void* nullable const pointer, const unsigned long size);
    void (* free)(void* nullable const);
} Allocator;

extern const Allocator DEFAULT_ALLOCATOR;
#define DEFAULT_ALLOCATOR &DEFAULT_ALLOCATOR

typedef void (* Deallocator)(void* const);

typedef void* (* Duplicator)(const void* const);

typedef enum : int /* not char for compatibility with stdlib's bsearch and qsort */ { // means: the first argument is [less, equal or greater] than the second one
    COMPARED_LESS = -1,
    COMPARED_EQUAL = 0,
    COMPARED_GREATER = 1
} Compared;

typedef Compared (* Comparator)(const void* const, const void* const); // a=first < b=second : negative, a = b : zero, a > b : positive; the parameters are actually void** - they're pointers to the values (which are pointers to smth too), left as single pointers for compatibility standard library

xinline void* xmemset(void* const destination, const int value, const unsigned long length) {
    void* memset(void* const, const int, const unsigned long);
    return memset(destination, value, length);
}

xinline void* xmemcpy(void* const destination, const void* const source, const unsigned long length) {
    void* memcpy(void* const, const void* const, const unsigned long);
    return memcpy(destination, source, length);
}

xinline void* xmemmove(void* const destination, const void* const source, const unsigned long length) {
    void* memmove(void* const, const void* const, const unsigned long);
    return memmove(destination, source, length);
}

xinline int xmemcmp(const void* const source1, const void* const source2, const unsigned long length) {
    int memcmp(const void* const, const void* const, const unsigned long);
    return memcmp(source1, source2, length);
}

xinline void xyield(void) {
    void thrd_yield(void);
    thrd_yield();
}

xinline unsigned long xstrnlen(const char* const string, const unsigned long maxSize) {
    unsigned long strnlen(const char* const, const unsigned long);
    return strnlen(string, maxSize);
}

int printf(const char* const, ...); // NOLINT(*-redundant-declaration)
int puts(const char* const); // NOLINT(*-redundant-declaration)
#define putsf(x, ...) printf(x "\n", __VA_ARGS__)

typedef enum {PRINT_MEMORY_MODE_DEC, PRINT_MEMORY_MODE_HEX, PRINT_MEMORY_MODE_HEX_STR, PRINT_MEMORY_MODE_STR, PRINT_MEMORY_MODE_TRY_STR_HEX_FALLBACK} PrintMemoryMode;
void printMemory(const void* const memory, const int size, const PrintMemoryMode mode);

// TODO: mock and unit test functions that uses 3rd party libraries by providing mock implementations of the libraries' functions simply defining those functions as they are weak symbols

void patchFunction(void* const original, void* const replacement); // overrides first 12 bytes of the original function with a trampoline to the replacement function

int hashValue(const void* const value, const int size); // fast non-cryptographic collision-resistant general purpose hash function (xxhash)
#define hashPrimitive(x) hashValue(&(typeof(x)) {x}, sizeof x) // _Generic((x), byte: 1, char: 1, unsigned short: 2, short: 2, unsigned int: 4, int: 4, unsigned long: 8, long: 8)

// TODO: add logger with various logging modes; add dynamic memory allocation tracker - store allocated memory addresses and corresponding addresses of *alloc callers; render lvgl via opengl optimized textures via embedded support via lvgl's generic opengl driver
// TODO: make stack and queue use (double) linked list instead of growable array, and make a 'fast' list - also utilizing (double) linked list - create a deque

// TODO: separate crypto routines into a standalone library
// TODO: separate networking module into a standalone library

// TODO: add arena allocator and fixed-size object allocator
// TODO: replace existing with recursive rw mutex

// TODO: read /proc/mappings for tracking allocations; check whether sdl truly replaces its own *alloc funcs with supplied once - leak sanitizer reports there are leaks caused by sdl and our mechanism reports the opposite

// TODO: select 'n poll - linux io multiplexing

// TODO: create a shared/standard comparator object and an enum for the results and use this one common object in all collections
// TODO: create a shared/standard deallocator object for the use with collections

// TODO: linux kernel's epoll & select system - use for io multiplexing
// TODO: create a coroutines implementation

// TODO: interact with trusted platform module to store the keys

// TODO: add/use opengl for drawing

// TODO: use ipv6
