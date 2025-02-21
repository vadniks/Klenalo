
#include <stdlib.h>
#include "defs.h"

static atomic unsigned long gAllocations = 0;

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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection" // CLion doesn't fully support new C23's attributes syntax
[[gnu::used]] void* nullable __wrap_lv_malloc_core(const unsigned long size) { return xmalloc(size); }
[[gnu::used]] void* nullable __wrap_lv_realloc_core(void* nullable const pointer, const unsigned long size) { return xrealloc(pointer, size); }
[[gnu::used]] void __wrap_lv_free_core(void* nullable const memory) { xfree(memory); }
#pragma clang diagnostic pop
