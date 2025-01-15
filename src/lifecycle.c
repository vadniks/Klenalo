
#include <SDL2/SDL.h>
#include <time.h>
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

typedef enum {
    LOOPER_ASYNC,
    LOOPER_MAIN
} Looper;

static const int UPDATE_PERIOD = 16; // floorf(1000.0f / 60.0f)

static atomic bool gInitialized = false;
static atomic bool gRunning = false;

static struct {
    List* queue; // <AsyncAction*>
    SDL_mutex* mutex; // for the queue
    SDL_Thread* thread;
}
    gAsyncActionsLooper = {nullptr, nullptr, nullptr}, // TODO: rename async to background
    gMainActionsLooper = {nullptr, nullptr, nullptr};

static int asyncActionsThreadLoop(void* const);

void lifecycleInit(void) {
    assert(!gInitialized);
    gInitialized = true;

    assert(SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0"));
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_TIMER) == 0);

    gAsyncActionsLooper.queue = listCreate(SDL_free);
    assert(gAsyncActionsLooper.mutex = SDL_CreateMutex());
    assert(gAsyncActionsLooper.thread = SDL_CreateThread(asyncActionsThreadLoop, "asyncActions", nullptr));

    gMainActionsLooper.queue = listCreate(SDL_free);
    assert(gMainActionsLooper.mutex = SDL_CreateMutex());

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

static int asyncActionsThreadLoop(void* const) { // TODO: rename
    while (!gRunning); // wait for main thread to start looping

    AsyncAction* action = nullptr;
    unsigned startMillis;

    while (gRunning) {
        assert(startMillis = SDL_GetTicks());

        assert(!SDL_LockMutex(gAsyncActionsLooper.mutex));
        if (listSize(gAsyncActionsLooper.queue))
            action = listPopFirst(gAsyncActionsLooper.queue);
        assert(!SDL_UnlockMutex(gAsyncActionsLooper.mutex));

        if (action) {
            if (action->delayMillis) SDL_Delay(action->delayMillis);
            if (gRunning) action->function(action->parameter);
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

unsigned long lifecycleCurrentTimeMillis(void) {
    struct timespec timespec;
    assert(!clock_gettime(CLOCK_REALTIME, &timespec));
    return (unsigned long) timespec.tv_sec * 1000ul + (unsigned long) timespec.tv_nsec / 1000000ul;
}

static void scheduleAction(const LifecycleAsyncActionFunction function, void* nullable const parameter, const int delayMillis, const Looper looper) {
    assert(gInitialized);

    AsyncAction* const action = SDL_malloc(sizeof *action);
    SDL_memcpy(action, &(AsyncAction) {function, parameter, delayMillis}, sizeof *action);

    SDL_mutex* const mutex = looper == LOOPER_ASYNC ? gAsyncActionsLooper.mutex : gMainActionsLooper.mutex;

    assert(!SDL_LockMutex(mutex));
    listAddFront(looper == LOOPER_ASYNC ? gAsyncActionsLooper.queue : gMainActionsLooper.queue, action);
    assert(!SDL_UnlockMutex(mutex));
}

void lifecycleRunAsync(const LifecycleAsyncActionFunction function, void* nullable const parameter, const int delayMillis) {
    scheduleAction(function, parameter, delayMillis, LOOPER_ASYNC);
}

void lifecycleRunInMainThread(const LifecycleAsyncActionFunction function, void* nullable const parameter) {
    scheduleAction(function, parameter, 0, LOOPER_MAIN);
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

        AsyncAction* action = nullptr; // TODO: move outside and unify

        assert(!SDL_LockMutex(gMainActionsLooper.mutex));
        if (listSize(gMainActionsLooper.queue))
            action = listPopFirst(gMainActionsLooper.queue);
        assert(!SDL_UnlockMutex(gMainActionsLooper.mutex));

        if (action) {
            action->function(action->parameter);
            SDL_free(action);
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

    SDL_DestroyMutex(gMainActionsLooper.mutex);
    listDestroy(gMainActionsLooper.queue);

    SDL_WaitThread(gAsyncActionsLooper.thread, nullptr);
    SDL_DestroyMutex(gAsyncActionsLooper.mutex);
    listDestroy(gAsyncActionsLooper.queue);

    SDL_Quit();
}
