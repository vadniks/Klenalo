
#include "lifecycle.h"
#include <SDL3/SDL.h>

static bool xSDL_Init(const SDL_InitFlags) {
    debug("patch")
    return false;
}

int main(void) {
    patchFunction(SDL_Init, xSDL_Init);
    return SDL_Init(0);

    const unsigned long allocations = xallocations();
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    assert(xallocations() == allocations);
    return 0;
}
