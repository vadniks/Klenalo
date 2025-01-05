
#if TESTING
#include <SDL2/SDL.h>
#include "list.h"
#include "hashtable.h"
#else
#include "lifecycle.h"
#endif

int main(
#if TESTING
    const int argc, const char* const* const argv
#else
    void
#endif
) {

#if TESTING
    assert(argc == 2);
    assert(!SDL_Init(0));

    switch (SDL_atoi(argv[1])) {
        case 0: listRunTests(); break;
        case 1: hashtableRunTests(); break;
    }

    SDL_Quit();
#else
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
#endif

    return 0;
}
