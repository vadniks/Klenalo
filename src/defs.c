
#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syslog.h>
#include <link.h>
#include <pthread.h>
#include <xxHash/xxhash.h>
#include "collections/treeMap.h"
#include "defs.h"

#ifdef DEBUG
typedef struct {
    unsigned long caller, memory, size;
} Allocation;

static TreeMap* gAllocationsTreeMap = nullptr;
static pthread_rwlock_t gAllocationsMapRwLock = PTHREAD_RWLOCK_INITIALIZER; // not our own RwMutex (SDL_ReadWriteLock under the hood) cuz it uses the tracked allocator
static const Allocator NON_TRACKED_ALLOCATOR = {malloc, calloc, realloc, free};
#endif // DEBUG

static atomic unsigned long gAllocations = 0;

#ifdef __clang__
[[maybe_unused]] void _deferHandler(void (^ const* const block)(void)) {
    (*block)();
}
#endif

static void printStackTrace(void) {
    int addressesSize = 0xf;
    const void* addresses[addressesSize];

    addressesSize = backtrace((void**) addresses, addressesSize);
    char** const strings = backtrace_symbols((void**) addresses, addressesSize);
    if (!strings) return;

    for (int i = 2; i < addressesSize; i++)
        fprintf(stderr, "%s\n", strings[i]),
        syslog(LOG_ERR, "%s\n", strings[i]);

    free(strings);
}

export void assert(const bool condition) {
    if (condition) return;

    const char message[] = "Assert failed at %p\n";
    fprintf(stderr, message, returnAddr);
    syslog(LOG_ERR, message, returnAddr);

    printStackTrace();
    abort(); // or __builtin_trap()
}

#ifdef DEBUG
[[gnu::constructor(1)]] used static void init(void) {
    gAllocationsTreeMap = treeMapCreate(&NON_TRACKED_ALLOCATOR, false, free);
}

[[gnu::destructor(1)]] used static void quit(void) {
    treeMapDestroy(gAllocationsTreeMap);
}

static int dlIteratePhdrCallback(struct dl_phdr_info* const info, const size_t, void* const data) {
    unsigned long* const xdata = data;
    if (!*xdata || !info->dlpi_name[0]) *xdata = info->dlpi_addr;
    return 0;
}

void checkUnfreedAllocations(void) {
    if (!gAllocations) return;

    unsigned long executableStartAddress = 0;
    dl_iterate_phdr(dlIteratePhdrCallback, &executableStartAddress);

    const char message[] = "Unfreed allocations found: %lu\n";
    fprintf(stderr, message, gAllocations);
    syslog(LOG_ERR, message, gAllocations);

    pthread_rwlock_rdlock(&gAllocationsMapRwLock);
    TreeMapIterator* iterator;
    treeMapIterateBegin(gAllocationsTreeMap, iterator);

//    used extern const byte __executable_start; // void* ... = &__executable_start; // linker trick, works on PIE too

    const byte bufSize = 0xff;
    char buf[bufSize];

    Allocation* allocation;
    while ((allocation = treeMapIterate(iterator))) {
        snprintf(
            buf, bufSize,
            "\tunfreed allocation caller=0x%lx (<elf>+0x%lx) memory=0x%lx size=%lu\n",
            allocation->caller,
            (byte) (allocation->caller >> 44ul) == 5 ? allocation->caller - executableStartAddress : -1ul, // if true - it's inside this executable, otherwise it's inside one of its shared libraries
            allocation->memory, allocation->size
        );

        fputs(buf, stderr);
        syslog(LOG_ERR, "%s", buf);
    }

    treeMapIterateEnd(iterator);
    pthread_rwlock_unlock(&gAllocationsMapRwLock);

    abort();
}

static void addAllocation(const unsigned long caller, const unsigned long memory, const unsigned long size) {
    Allocation* const allocation = malloc(sizeof *allocation);
    assert(allocation);
    allocation->caller = caller;
    allocation->memory = memory;
    allocation->size = size;

    pthread_rwlock_wrlock(&gAllocationsMapRwLock);
    treeMapInsert(gAllocationsTreeMap, hashPrimitive(memory), allocation);
    pthread_rwlock_unlock(&gAllocationsMapRwLock);
}
#define addAllocation(x, y) addAllocation((unsigned long) returnAddr, (unsigned long) x, y)

static void removeAllocation(const unsigned long memory) {
    pthread_rwlock_wrlock(&gAllocationsMapRwLock);
    treeMapDelete(gAllocationsTreeMap, hashPrimitive(memory));
    pthread_rwlock_unlock(&gAllocationsMapRwLock);
}
#define removeAllocation(x) removeAllocation((unsigned long) x)
#else // DEBUG
void checkUnfreedAllocations(void) {
    assert(!gAllocations);
}
#endif // DEBUG

void* xmalloc(const unsigned long size) {
    return xcalloc(size, 1);
}

void* xcalloc(const unsigned long elements, const unsigned long size) {
    void* const memory = calloc(elements, size);
    assert(memory);
    gAllocations++;

#ifdef DEBUG
    addAllocation(memory, elements * size);
#endif
    return memory;
}

void* nullable xrealloc(void* nullable const pointer, const unsigned long size) {
    void* const memory = realloc(pointer, size);

    if (!pointer) { // !pointer && size
        assert(memory);
        gAllocations++;

#ifdef DEBUG
        addAllocation(memory, size);
#endif
    } else if (size) { // pointer && size
        assert(memory);

#ifdef DEBUG
        removeAllocation(pointer);
        addAllocation(memory, size);
#endif
    } else { // pointer && !size
        assert(!memory);
        gAllocations--;

#ifdef DEBUG
        removeAllocation(pointer);
#endif
    }

    return memory;
}

void xfree(void* nullable const memory) {
    free(memory);
    if (!memory) return;
    gAllocations--;

#ifdef DEBUG
    removeAllocation(memory);
#endif
}

#undef DEFAULT_ALLOCATOR
const Allocator DEFAULT_ALLOCATOR = {xmalloc, xcalloc, xrealloc, xfree};

void printMemory(const void* const memory, const int size, const PrintMemoryMode mode) {
    const char* format, * divider;
    bool tryStr = false;

    switch (mode) {
        case PRINT_MEMORY_MODE_DEC:
            format = "%u";
            divider = ", ";
            break;
        case PRINT_MEMORY_MODE_HEX:
            format = "%x";
            divider = ", ";
            break;
        case PRINT_MEMORY_MODE_HEX_STR:
            format = "\\x%x";
            divider = "";
            break;
        case PRINT_MEMORY_MODE_STR:
            format = "%c";
            divider = "";
            break;
        case PRINT_MEMORY_MODE_TRY_STR_HEX_FALLBACK:
            tryStr = true;
            break;
    }

    if (tryStr) {
        for (int i = 0; i < size; i++) {
            const byte bt = ((const byte*) memory)[i];
            bt == ' ' || inRange('a', bt, 'z') || inRange('A', bt, 'Z') || inRange('0', bt, '9')
                ? printf("\033[1;32m%c\033[0m", bt) : printf("%x", bt);
            if (i < size - 1) printf(" ");
        }
    } else
        for (int i = 0; i < size; printf(format, ((const byte*) memory)[i++]), i < size ? printf("%s", divider) : 0);
    printf("\n");
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-internal"
static void trampoline(void);
asm(
    "trampoline:\n"
    // TODO: add endbr64 or just skip first 4 bytes in the target function
    // if this function gets called accidentally, it just fails, these bytes are ignored
    "xorl %edi,%edi\n" // 2 bytes
    "jmp assert\n" // 5 bytes
    // these bytes are copied and the address is replaced with the actual one
    "movabsq $-1,%rax\n" // 2 + 8 bytes
    "jmpq *%rax" // 2 bytes
);
#pragma clang diagnostic pop

void patchFunction(void* const original, void* const replacement) {
    const unsigned long pageSize = sysconf(_SC_PAGESIZE);
    void* const pageStart = (void*) ((unsigned long) original & ~(pageSize - 1)); // the same as (void*) ((unsigned long) original / pageSize) * pageSize

    assert(!mprotect(pageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC));

    xmemcpy(original, (void*) trampoline + 7, 12);
    xmemcpy(original + 2, &replacement, 8);

    assert(!mprotect(pageStart, pageSize, PROT_READ | PROT_EXEC));
}

int hashValue(const void* const value, const int size) {
    return (int) XXH32(value, size, 0);
}
