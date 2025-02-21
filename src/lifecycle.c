
#include <SDL3/SDL.h>
#include "xlvgl.h"
#include "video.h"
#include "input.h"
#include "resources.h"
#include "scenes.h"
#include "net.h"
#include "list.h"
#include "lifecycle.h"

typedef struct {
    const LifecycleAsyncActionFunction function;
    void* nullable const parameter;
    const int delayMillis;
} AsyncAction;

typedef enum {
    LOOPER_BACKGROUND,
    LOOPER_MAIN
} Looper;

static const int UPDATE_PERIOD = 16; // floorf(1000.0f / 60.0f)

static atomic bool gInitialized = false;
static atomic bool gRunning = false;

static RWMutex* gUIRWMutex = nullptr;

static struct {
    List* nullable queue; // <AsyncAction*>
    SDL_Thread* nullable thread;
}
    gMainActionsLooper = {nullptr, nullptr},
    gBackgroundActionsLooper = {nullptr, nullptr},
    gNetActionsLooper = {nullptr, nullptr};

static unsigned int getTicks(void);
static int backgroundActionsLoop(void* const);
static int netActionsLoop(void* const);

void lifecycleInit(void) {
    assert(!gInitialized);
    gInitialized = true;

    // lvgl memory functions? TODO
    assert(SDL_SetMemoryFunctions(xmalloc, xcalloc, xrealloc, xfree));
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    lv_init();
    lv_tick_set_cb(getTicks);
    lv_delay_set_cb(SDL_Delay);

    gUIRWMutex = rwMutexCreate();

    gMainActionsLooper.queue = listCreate(true, xfree);
    gBackgroundActionsLooper.queue = listCreate(true, xfree);

    videoInit();
    inputInit();
    resourcesInit();
    scenesInit();
    netInit();

    assert(gBackgroundActionsLooper.thread = SDL_CreateThread(backgroundActionsLoop, "backgroundActions", nullptr));
    assert(gNetActionsLooper.thread = SDL_CreateThread(netActionsLoop, "netActions", nullptr));
}

static unsigned int getTicks(void) {
    return SDL_GetTicks() & 0xffffffff;
}

static void delayThread(const unsigned startMillis) {
    const unsigned differenceMillis = SDL_GetTicks() - startMillis;
    if (UPDATE_PERIOD > differenceMillis)
        SDL_Delay(UPDATE_PERIOD - differenceMillis);
}

static AsyncAction* nullable nextAsyncAction(const Looper looper) {
    return listPopFirst(looper == LOOPER_BACKGROUND ? gBackgroundActionsLooper.queue : gMainActionsLooper.queue);
}

static void threadLoop(void (* const body)(void)) {
    while (!gRunning); // wait for main thread to start looping

    unsigned startMillis;

    while (gRunning) {
        assert(startMillis = SDL_GetTicks());
        body();
        delayThread(startMillis);
    }
}

static void backgroundActionsLoopBody(void) {
    AsyncAction* action;
    if ((action = nextAsyncAction(LOOPER_BACKGROUND))) {
        if (action->delayMillis) SDL_Delay(action->delayMillis);
        if (gRunning) action->function(action->parameter);
        xfree(action);
    }
}

static int backgroundActionsLoop(void* const) {
    threadLoop(backgroundActionsLoopBody);
    return 0;
}

static void netLoopBody(void) {
    netListen();
}

static int netActionsLoop(void* const) {
    threadLoop(netLoopBody);
    return 0;
}

bool lifecycleInitialized(void) {
    return gInitialized;
}

unsigned long lifecycleCurrentTimeMillis(void) {
    SDL_Time ticks;
    assert(SDL_GetCurrentTime(&ticks));
    return ticks / 1'000'000ul;
}

static void scheduleAction(const LifecycleAsyncActionFunction function, void* nullable const parameter, const int delayMillis, const Looper looper) {
    assert(gInitialized);

    AsyncAction* const action = xmalloc(sizeof *action);
    assert(action);
    xmemcpy(action, &(AsyncAction) {function, parameter, delayMillis}, sizeof *action);

    listAddFront(looper == LOOPER_BACKGROUND ? gBackgroundActionsLooper.queue : gMainActionsLooper.queue, action);
}

void lifecycleRunInBackground(const LifecycleAsyncActionFunction function, void* nullable const parameter, const int delayMillis) {
    scheduleAction(function, parameter, delayMillis, LOOPER_BACKGROUND);
}

void lifecycleRunInMainThread(const LifecycleAsyncActionFunction function, void* nullable const parameter) {
    scheduleAction(function, parameter, 0, LOOPER_MAIN);
}

void lifecycleUIMutexCommand(const RWMutexCommand command) {
    assert(gInitialized);
    rwMutexCommand(gUIRWMutex, command);
}

void lifecycleLoop(void) {
    assert(gInitialized);
    gRunning = true;

    unsigned startMillis;
    AsyncAction* action;

    while (true) {
        assert(startMillis = SDL_GetTicks());

        rwMutexWriteLock(gUIRWMutex);
        lv_timer_periodic_handler();

        SDL_Event event;
        while (SDL_PollEvent(&event) == 1) {
            if (event.type == SDL_EVENT_QUIT) {
                rwMutexWriteUnlock(gUIRWMutex);
                goto end;
            }

            videoProcessEvent(&event);
            inputProcessEvent(&event);
        }
        rwMutexWriteUnlock(gUIRWMutex);

        if ((action = nextAsyncAction(LOOPER_MAIN))) {
            action->function(action->parameter);
            xfree(action);
        }

        delayThread(startMillis);
    }
    end:

    gRunning = false;
}

void lifecycleQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    SDL_WaitThread(gNetActionsLooper.thread, nullptr);
    SDL_WaitThread(gBackgroundActionsLooper.thread, nullptr);

    netQuit();
    scenesQuit();
    resourcesQuit();
    inputQuit();
    videoQuit();

    listDestroy(gBackgroundActionsLooper.queue);
    listDestroy(gMainActionsLooper.queue);

    rwMutexDestroy(gUIRWMutex);

    lv_deinit();

    SDL_Quit();
}
