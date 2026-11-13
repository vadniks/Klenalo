
#pragma once

//#define TESTING

#if __STDC_VERSION__ < 202311L /*C23*/ || !defined(__clang__) /*extensions*/ || !defined(__GNUC__) /*glibc*/ || \
    !defined(__linux__) /*api*/ || !defined(_GNU_SOURCE) /*api*/ || !defined(__x86_64__) || !__LITTLE_ENDIAN__ || \
    (defined(__WORDSIZE) && __WORDSIZE != 64 || false) || \
    !defined(__SSE4_2__) || !defined(__AVX2__) /* where's avx512, intel?! */ || !defined(__POPCNT__) || !defined(__BMI2__)
#   error
#endif

typedef unsigned char byte; // none
typedef unsigned short ushort; // word
// typedef unsigned int uint; // longword/doubleword
typedef unsigned long ulong; // quadword
typedef unsigned long uintptr; // void* // TODO: replace void* in collections with uintptr
typedef __int128 octaword;
typedef unsigned __int128 uoctaword;
typedef unsigned char ufast8;
typedef unsigned long ufast16;
typedef unsigned long ufast32;
typedef unsigned long ufast64;

#define PROGRAM_NAME "Klenalo"

// everything (function return types, function parameter type, struct field type) that isn't marked with nullable is considered to be non-null
#define nullable _Nullable

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
#define offsetof(x, y) __builtin_offsetof(x, y)
#define export [[gnu::visibility("default")]]
#define noinline [[clang::noinline]]
#define xinline [[clang::always_inline]] inline
#define popcount(x) __builtin_popcount(x)
#define typed(x, y) x
#define bswap(x) _Generic((x), \
    short: __builtin_bswap16, unsigned short: __builtin_bswap16, \
    int: __builtin_bswap32, unsigned int: __builtin_bswap32, \
    long: __builtin_bswap64, unsigned long: __builtin_bswap64)(x)

#define used [[gnu::used]]

// or just xmemcpy(x, &(typeof(*x)) {__VA_ARGS__}, sizeof *x); in case this is a mess
#define assignToStructWithConsts(x, ...) { \
    struct packed _S {byte _[sizeof(typeof(*x))];}; \
    staticAssert(sizeof(struct _S) == sizeof(typeof(*x))); \
    *((struct _S*) x) = *(struct _S*) &(typeof(*x)) {__VA_ARGS__}; \
} // struct __ANY_STRUCT_TYPE_* const var = xmalloc(sizeof *var); if that struct contains const qualified fields; x is var (pointer to that struct)

staticAssert(sizeof(char) == 1 & sizeof(short) == 2 & sizeof(int) == 4 & sizeof(long) == 8 & sizeof(long long) == 8 & sizeof(void*) == 8);

void assert(bool condition);
int xrand(int min, int max); // general pseudorandom

typedef void (* Deallocator)(void*);

typedef void* (* Duplicator)(void*);

typedef enum : int /* not char for compatibility with stdlib's bsearch and qsort */ { // means: the first argument is [less, equal or greater] than the second one
    COMPARED_LESS = -1,
    COMPARED_EQUAL = 0,
    COMPARED_GREATER = 1
} Compared;

typedef Compared (* Comparator)(const void*, const void*); // a=first < b=second : negative, a = b : zero, a > b : positive; the parameters are actually meant to be void** - they're pointers to the values (which are pointers to smth too), left as single pointers for compatibility standard library; the first argument is always a pointer to the next value in the container (iterated) and the second one is the value supplied as the key (use &(<arg>) to put a pointer to it)

void syslog(int priority, const char* format, ...); // NOLINT(*-redundant-declaration, *-inconsistent-declaration-parameter-name)
#define log(x, y) syslog(7, x, y) // LOG_DEBUG

typedef enum {
    PRINT_MEMORY_MODE_DEC,
    PRINT_MEMORY_MODE_HEX,
    PRINT_MEMORY_MODE_HEXDUMP,
    PRINT_MEMORY_MODE_BINARY,
    PRINT_MEMORY_MODE_HEX_STR,
    PRINT_MEMORY_MODE_STR,
    PRINT_MEMORY_MODE_IPV6,
    PRINT_MEMORY_MODE_TRY_STR_HEX_FALLBACK
} PrintMemoryMode;
void printMemory(const void* memory, int size, PrintMemoryMode mode, bool newline);

int hashValue(const void* value, int size); // fast collision-resistant general purpose hash function (xxhash32)
inline int hashPrimitive(const long value) { return hashValue(&value, sizeof(value)); } // byte/char, (u)short, (u)int cast to long automatically (compiler just takes lower bytes registers of the main 8 bytes register)

ulong currentTimeMillis();

// TODO: put restrict everywhere where applicable

int printf(const char* restrict format, ...);
int puts(const char* s);

void* malloc(ulong size); // assume returns nonnull as there's plenty of virtual memory in modern systems
void* calloc(ulong num, ulong size); // TODO: assert *allocs
void* realloc(void* ptr, ulong size);
void free(void* ptr);

void* memset(void* s, int c, ulong n);
void* memmove(void* dest, const void* src, ulong n);
void* memcpy(void* restrict dest, const void* restrict src, ulong n);

typedef byte vec16ab [[gnu::vector_size(16), gnu::aligned(16)]]; // aligned xmm vector[1-16] of 16 bytes represented just as bytes
typedef short vec16aw [[gnu::vector_size(16), gnu::aligned(16)]]; // aligned xmm vector[1-8] of 16 bytes represented as 4 packed words (shorts)
typedef int vec16al [[gnu::vector_size(16), gnu::aligned(16)]]; // aligned xmm vector[1-4] of 16 bytes represented as 4 packed longwords (ints)
typedef long vec16aq [[gnu::vector_size(16), gnu::aligned(16)]]; // aligned xmm vector[1-2] of 16 bytes represented as 2 packed quadwords (longs)

bool compareXmmVectors(vec16aq vec1, vec16aq vec2);
