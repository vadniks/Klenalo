
#if TESTING
#   include <SDL3/SDL.h>
#   include "list.h"
#   include "hashtable.h"
#else
#   include "lifecycle.h"
#endif

int main(void) {

#if TESTING
    assert(!SDL_Init(0));

    const unsigned long allocations = xallocations();

    listRunTests();
    hashtableRunTests();

    assert(xallocations() == allocations);

    SDL_Quit();
#else
    const unsigned long allocations = xallocations();
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    assert(xallocations() == allocations);
#endif
    return 0;
}
