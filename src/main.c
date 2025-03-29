
#include "lifecycle.h"
#include <stdio.h>

static FILE* xfopen(const char* const filename, const char* const modes) {
    debugArgs("patched fopen(%s, %s)", filename, modes)
    return nullptr;
}

int main(void) {
    patchFunction((void*) fopen, (void*) xfopen);

    FILE* const self = fopen("/proc/self/exe", "rb");
    assert(!self);

    return 0;

    const unsigned long allocations = xallocations();
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    assert(xallocations() == allocations);
    return 0;
}
