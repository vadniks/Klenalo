
#include "lifecycle.h"

#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <SDL3/SDL.h>

[[gnu::noinline]]
static bool xSDL_Init(SDL_InitFlags flags) {
    debug("aaa")
    return false;
}

static void patchFunction(void* const original, void* const replacement) {
    const unsigned long pageSize = sysconf(_SC_PAGESIZE);
    void* const pageStart = (void*) ((unsigned long) original & ~(pageSize - 1));

    assert(!mprotect(pageStart, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC));

    byte trampoline[12] = {0x48, 0xb8, [10] = 0xff, [11] = 0xe0};
    *(unsigned long*) &trampoline[2] = (unsigned long) replacement;
    memcpy(original, trampoline, sizeof(trampoline));

    assert(!mprotect(pageStart, pageSize, PROT_READ | PROT_EXEC));
}

int main(void) {
    debugArgs("%p %p", SDL_Init, xSDL_Init)
    patchFunction((void*) SDL_Init, (void*) xSDL_Init);
    debugArgs("%c", SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) ? 't' : 'f')
    return 0;

    const unsigned long allocations = xallocations();
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    assert(xallocations() == allocations);
    return 0;
}
