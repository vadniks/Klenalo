
#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/syslog.h>
#include "defs.h"

static atomic unsigned long gAllocations = 0;

#ifdef __clang__
[[maybe_unused]] void _deferHandler(void (^ const* const block)(void)) {
    (*block)();
}
#endif

static void printStackTrace(void) {
    const int size = 0xf;
    void* array[size];

    const int actualSize = backtrace(array, size);
    char** const strings = backtrace_symbols(array, actualSize);
    if (!strings) return;

    for (int i = 3; i < actualSize; i++)
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
    abort();
}

unsigned long xallocations(void) {
    const char message[] = "Allocations: %lu\n";
    fprintf(stderr, message, gAllocations);
    syslog(LOG_ERR, message, gAllocations);
    return gAllocations;
}

void* nullable xmalloc(const unsigned long size) {
    return xcalloc(size, 1);
}

void* nullable xcalloc(const unsigned long elements, const unsigned long size) {
    void* const memory = calloc(elements, size);
    if (memory) gAllocations++;
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

    return memory;
}

void xfree(void* nullable const memory) {
    free(memory);
    if (memory) gAllocations--;
}

const Allocator ALLOCATOR_DEFAULT = {xmalloc, xcalloc, xrealloc, xfree};

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

int hashValue(const byte* value, int size) {
    int hash = 0;
    for (; size--; hash = 31 * hash + *value++);
    return hash;
}
