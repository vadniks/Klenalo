
#include <SDL2/SDL.h>
#include "xlvgl.h"
#include "video.h"
#include "input.h"
#include "resources.h"
#include "scenes.h"
#include "list.h"
#include "lifecycle.h"

typedef struct {
    const LifecycleAsyncActionFunction function;
    void* nullable const parameter;
    const int delayMillis;
} AsyncAction;

static const int UPDATE_PERIOD = 16; // floorf(1000.0f / 60.0f)

static atomic bool gInitialized = false;
static atomic bool gRunning = false;
static List* gAsyncActionsQueue = nullptr; // <AsyncAction*>
static SDL_mutex* gAsyncActionsQueueMutex = nullptr;
static SDL_Thread* gAsyncActionsThread = nullptr;

static int asyncActionsThreadLoop(void* const);

void lifecycleInit(void) {
    assert(!gInitialized);
    gInitialized = true;

    assert(SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0"));
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == 0);

    gAsyncActionsQueue = listCreate(SDL_free);
    assert(gAsyncActionsQueueMutex = SDL_CreateMutex());
    assert(gAsyncActionsThread = SDL_CreateThread(asyncActionsThreadLoop, "asyncActions", nullptr));

    lv_init();
    lv_tick_set_cb(SDL_GetTicks);
    lv_delay_set_cb(SDL_Delay);

    videoInit();
    inputInit();
    resourcesInit();
    scenesInit();
}

static void delayThread(const unsigned startMillis) {
    const unsigned differenceMillis = SDL_GetTicks() - startMillis;
    if (UPDATE_PERIOD > differenceMillis)
        SDL_Delay(UPDATE_PERIOD - differenceMillis);
}

static int asyncActionsThreadLoop(void* const) {
    while (!gRunning); // wait for main thread to start looping

    AsyncAction* action = nullptr;
    unsigned startMillis;

    while (gRunning) {
        assert(startMillis = SDL_GetTicks());

        assert(!SDL_LockMutex(gAsyncActionsQueueMutex));
        if (listSize(gAsyncActionsQueue))
            action = listPopFirst(gAsyncActionsQueue);
        assert(!SDL_UnlockMutex(gAsyncActionsQueueMutex));

        if (action) {
            if (action->delayMillis) SDL_Delay(action->delayMillis);
            action->function(action->parameter);
            SDL_free(action);
            action = nullptr;
        }

        delayThread(startMillis);
    }

    return 0;
}

bool lifecycleInitialized(void) {
    return gInitialized;
}

void lifecycleAsync(const LifecycleAsyncActionFunction function, void* nullable const parameter, int delayMillis) {
    assert(gInitialized);

    AsyncAction* const action = SDL_malloc(sizeof *action);
    SDL_memcpy(action, &(AsyncAction) {function, parameter, delayMillis}, sizeof *action);

    assert(!SDL_LockMutex(gAsyncActionsQueueMutex));
    listAddFront(gAsyncActionsQueue, action);
    assert(!SDL_UnlockMutex(gAsyncActionsQueueMutex));
}

void lifecycleLoop(void) {
    assert(gInitialized);
    gRunning = true;

    unsigned startMillis;
    while (true) {
        assert(startMillis = SDL_GetTicks());
        lv_timer_periodic_handler();

        SDL_Event event;
        while (SDL_PollEvent(&event) == 1) {
            if (event.type == SDL_QUIT) goto end;

            videoProcessEvent(&event);
            inputProcessEvent(&event);
        }

        delayThread(startMillis);
    }
    end:

    gRunning = false;
}

void lifecycleQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    scenesQuit();
    resourcesQuit();
    inputQuit();
    videoQuit();

    lv_deinit();

    SDL_WaitThread(gAsyncActionsThread, nullptr);
    SDL_DestroyMutex(gAsyncActionsQueueMutex);
    listDestroy(gAsyncActionsQueue);

    SDL_Quit();
}
