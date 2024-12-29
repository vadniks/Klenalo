
#include <SDL2/SDL.h>
#include "xlvgl.h"
#include "defs.h"
#include "video.h"
#include "input.h"
#include "resources.h"
#include "scenes.h"
#include "lifecycle.h"

static const int UPDATE_PERIOD = 16; // floorf(1000.0f / 60.0f)

static atomic bool gInitialized = false;

void lifecycleInit(void) {
    assert(!gInitialized);
    gInitialized = true;

    assert(SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0"));
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0);

    lv_init();
    lv_tick_set_cb(SDL_GetTicks);
    lv_delay_set_cb(SDL_Delay);

    videoInit();
    inputInit();
    resourcesInit();
    scenesInit();
}

bool lifecycleInitialized(void) {
    return gInitialized;
}

void lifecycleLoop(void) {
    assert(gInitialized);

    unsigned startMillis, differenceMillis;
    while (true) {
        startMillis = SDL_GetTicks();
        lv_timer_periodic_handler();

        SDL_Event event;
        while (SDL_PollEvent(&event) == 1) {
            if (event.type == SDL_QUIT) return;

            videoProcessEvent(&event);
            inputProcessEvent(&event);
        }

        differenceMillis = SDL_GetTicks() - startMillis;
        if (UPDATE_PERIOD > differenceMillis)
            SDL_Delay(UPDATE_PERIOD - differenceMillis);
    }
}

void lifecycleQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    scenesQuit();
    resourcesQuit();
    inputQuit();
    videoQuit();

    lv_deinit();
    SDL_Quit();
}
