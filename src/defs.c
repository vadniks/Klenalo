
#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syslog.h>
#include <string.h>
#include "collections/treeMap.h"
#include "collections/list.h"
#include "rwMutex.h"
#include "defs.h"

#define functionFooterDeclaration \
    asm volatile ( \
        "jmp .x%=\n" \
        ".ascii \"footer_" __FUNCTION__ "\"\n" \
        ".x%=:\n" \
        "nop" \
    :::);

typedef struct {
    unsigned long caller, memory, size;
} Allocation;

static atomic unsigned long gAllocations = 0;
static RWMutex* atomic gAllocationsRwMutex = nullptr;
static TreeMap* atomic gAllocationsTreeMap = nullptr; // <Allocation*>
static List* atomic gAllocationsList = nullptr; // <Allocation*>

#ifdef __clang__
[[maybe_unused]] void _deferHandler(void (^ const* const block)(void)) {
    (*block)();
}
#endif

static void init(void) {
    gAllocationsRwMutex = rwMutexCreate();
    gAllocationsTreeMap = treeMapCreate(false, xfree);
    gAllocationsList = listCreate(false, xfree);
}

static void quit(void) {
    rwMutexWriteLock(gAllocationsRwMutex); // technically unnecessary

    TreeMap* const map = gAllocationsTreeMap;
    gAllocationsTreeMap = nullptr;
    treeMapDestroy(map);

    List* const list = gAllocationsList;
    gAllocationsList = nullptr;
    listDestroy(list);

    rwMutexWriteUnlock(gAllocationsRwMutex);

    RWMutex* const rwMutex = gAllocationsRwMutex;
    gAllocationsRwMutex = nullptr;

    rwMutexDestroy(rwMutex);
}

static void printStackTrace(void) {
    int size = 0xf;
    const void* array[size];

    size = backtrace((void**) array, size);
    char** const strings = backtrace_symbols((void**) array, size);
    if (!strings) return;

    for (int i = 0; i < size; i++)
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

static inline void xRwMutexCommand(RWMutex* const rwMutex, const RWMutexCommand command) {
    if (rwMutex) rwMutexCommand(rwMutex, command);
}

unsigned long xallocations(void) {
    const char message[] = "Allocations: %lu\n";
    fprintf(stderr, message, gAllocations);
    syslog(LOG_ERR, message, gAllocations);
    return gAllocations;
}

void xunfreedAllocations(void) {
    if (!gAllocations || !gAllocationsTreeMap || !gAllocationsList) return;

    xRwMutexCommand(gAllocationsRwMutex, RW_MUTEX_COMMAND_WRITE_LOCK);
    printf("unfreed allocations count: %lu %d %p %p\n", gAllocations, treeMapCount(gAllocationsTreeMap), gAllocationsTreeMap, gAllocationsRwMutex);

//    TreeMapIterator* iterator;
//    treeMapIterateBegin(gAllocationsTreeMap, iterator);
//    Allocation* allocation;
//    while ((allocation = treeMapIterate(iterator)))
//        printf("\tunfreed allocation: caller=%lx memory=%lx size=%lu\n", allocation->caller, allocation->memory, allocation->size);
//    treeMapIterateEnd(iterator);

    for (int i = 0; i < listSize(gAllocationsList); i++) {
        Allocation* const allocation = listGet(gAllocationsList, i);
        printf("\tunfreed allocation: caller=%lx memory=%lx size=%lu\n", allocation->caller, allocation->memory, allocation->size);
    }

    xRwMutexCommand(gAllocationsRwMutex, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

static bool recursion(const void* const functionStart, const char* const footer) {
    const int maxFunctionSize = 2048;
    const void* const functionEnd = memmem(
        functionStart + 4,
        maxFunctionSize,
        (const byte[4]) /*endbr64*/ {0xf3, 0x0f, 0x1e, 0xfa}, //footer,
        4 //strnlen(footer, 16)
    );
    assert(functionEnd);

    byte addressesSize = 0xf;
    const void* addresses[addressesSize];

    addressesSize = (byte) backtrace((void**) addresses, addressesSize);
    // i = 2 -> starts from where this function is called, i = 3 -> starts from where the caller of this function is called
    for (byte i = 3; i < addressesSize; i++) {
        printf("addr %p | %d | %p %p ", addresses[i], addressesSize, xfree, functionEnd);
        if (addresses[i] > functionStart && addresses[i] < functionEnd) {
            printf("1\n");
            return true;
        }
        else printf("0\n");
    }

    // cat /proc/self/maps
    // cat /proc/meminf
    // /proc/self/exe

    return false;
}
#define recursion(x) recursion((void*) x, "footer_" #x)

static int hashPointer(const void* nullable const pointer) {
    return hashValue((const void*) &pointer, sizeof pointer);
}

//void* nullable xmalloc(const unsigned long size) {
//    functionFooterDeclaration
//    return xcalloc(size, 1);
//}

noinline void* nullable xcalloc(const unsigned long elements, const unsigned long size) {
    void* const memory = calloc(elements, size);
    if (memory) gAllocations++;

    printf("h %d | %p %p %p\n", hashPointer(memory), gAllocationsTreeMap, memory, gAllocationsList);

    if (gAllocationsTreeMap && !recursion(xcalloc) && memory && gAllocationsList) {
        Allocation* const allocation = xmalloc(sizeof *allocation);
        assert(allocation);
        allocation->caller = (unsigned long) returnAddr;
        allocation->memory = (unsigned long) memory;
        allocation->size = elements * size;
//        printf("xc caller=%p memory=%p size=%lu\n", returnAddr, memory, elements * size);

        xRwMutexCommand(gAllocationsRwMutex, RW_MUTEX_COMMAND_WRITE_LOCK);
        listAddBack(gAllocationsList, allocation);
        printf("aaaaa\n");
//        treeMapInsert(gAllocationsTreeMap, hashPointer(memory), allocation);
//        Allocation* a = treeMapSearchKey(gAllocationsTreeMap, hashPointer(memory));
//        assert(a);
//        printf("ab hash=%d caller=%lx memory=%lx size=%lu\n", hashPointer(memory), a->caller, a->memory, a->size);
        xRwMutexCommand(gAllocationsRwMutex, RW_MUTEX_COMMAND_WRITE_UNLOCK);
    }

    functionFooterDeclaration
    return memory;
}

void* nullable xrealloc(void* nullable const pointer, const unsigned long size) {
    void* const memory = realloc(pointer, size);

    if (!pointer && memory)
        gAllocations++;
    else if (pointer && !size) {
        assert(!memory);
        gAllocations--;
    }

    functionFooterDeclaration
    return memory;
}

noinline void xfree(void* nullable const memory) {
    free(memory);
    if (memory) gAllocations--;

    if (gAllocationsTreeMap && !recursion(xfree) && memory && gAllocationsList) {
        printf("xfree norecursion\n");
        xRwMutexCommand(gAllocationsRwMutex, RW_MUTEX_COMMAND_WRITE_LOCK);
        for (int i = 0; i < listSize(gAllocationsList); i++) {
            if (((Allocation*) listGet(gAllocationsList, i))->memory == (unsigned long) memory) {
                printf("bbbbb\n");
                listRemove(gAllocationsList, i);
                break;
            }
        }
//        treeMapDelete(gAllocationsTreeMap, hashPointer(memory)); endbr64 = f3 0f 1e fa
        xRwMutexCommand(gAllocationsRwMutex, RW_MUTEX_COMMAND_WRITE_UNLOCK);
    } else printf("xfree recursion\n");
//asm volatile ("endbr64");
    functionFooterDeclaration
}

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
    // TODO: add endbr64
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

[[gnu::no_sanitize("unsigned-integer-overflow")]]
int hashValue(const byte* value, int size) {
    unsigned int hash = 0;
    for (; size--; hash = 31 * hash + *value++);
    return (int) hash;
}

// or just use [[gnu::constructor(1+)]]
void (* nullable const START_HOOK)(void) = init;
void (* nullable const END_HOOK)(void) = quit;
