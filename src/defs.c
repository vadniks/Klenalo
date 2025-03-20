
#include <stdlib.h>
#include <stdio.h>
#include <execinfo.h>
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
        fprintf(stderr, "%s\n", strings[i]);

    free(strings);
}

void assert(const bool condition) {
    if (condition) return;
    fputs("Assert failed\n", stderr);
    printStackTrace();
    abort();
}

unsigned long xallocations(void) {
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

void printMemory(const void* const memory, const int size, const PrintMemoryMode mode) {
    const char* format, * divider;
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
    }

    for (int i = 0; i < size; printf(format, ((byte*) memory)[i++]), i < size ? printf("%s", divider) : 0);
    printf("\n");
}

__attribute_used__ void* nullable __wrap_lv_malloc_core(const unsigned long size) { return xmalloc(size); }
__attribute_used__ void* nullable __wrap_lv_realloc_core(void* nullable const pointer, const unsigned long size) { return xrealloc(pointer, size); }
__attribute_used__ void __wrap_lv_free_core(void* nullable const memory) { xfree(memory); }
