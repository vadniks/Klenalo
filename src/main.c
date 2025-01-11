
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

    listRunTests();
    hashtableRunTests();

    SDL_Quit();
#else
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
#endif

    assert(!SDL_GetNumAllocations());
    return 0;
}
