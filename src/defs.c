
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
