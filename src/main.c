
#include <SDL2/SDL_stdinc.h>

#if TESTING
#   include <SDL2/SDL.h>
#   include "list.h"
#   include "hashtable.h"
#else
#   include "lifecycle.h"
#endif

int main(void) {

#if TESTING
    assert(!SDL_Init(0));

    const int allocations = SDL_GetNumAllocations();

    listRunTests();
    hashtableRunTests();

    assert(SDL_GetNumAllocations() == allocations);

    SDL_Quit();
#else
    const int allocations = SDL_GetNumAllocations();
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    assert(SDL_GetNumAllocations() == allocations);
#endif
    return 0;
}
