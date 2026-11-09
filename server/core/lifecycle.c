
#include <SDL3/SDL.h>
#include <stdio.h>
#include "../network/network.h"
#include "../../common/collections/deque.h"
#include "lifecycle.h"

typedef struct {
    const LifecycleAsyncActionFunction function;
    void* nullable const parameter;
    const int delayMillis;
} AsyncAction;

typedef enum {
    LOOPER_MAIN,
    LOOPER_BACKGROUND,
    LOOPER_NETWORK
} Looper;

static constexpr int UPDATE_PERIOD = 16; // floorf(1000.0f / 60.0f)

static atomic bool gInitialized = false;
static atomic bool gRunning = false;
static int gTicker = 0;

static struct {
    Deque* nullable queue; // <AsyncAction*>
    SDL_Thread* nullable thread;
}
    gMainActionsLooper = {nullptr, nullptr},
    gBackgroundActionsLooper = {nullptr, nullptr},
    gNetworkActionsLooper = {nullptr, nullptr};

static int backgroundActionsLoop(void* nullable);
static int networkActionsLoop(void* nullable);

void lifecycleInit() {
    assert(!gInitialized);
    gInitialized = true;

    assert(SDL_Init(SDL_INIT_EVENTS));

    gMainActionsLooper.queue = dequeCreate(free);

    gBackgroundActionsLooper.queue = dequeCreate(free);
    assert(gBackgroundActionsLooper.thread = SDL_CreateThread(backgroundActionsLoop, PROGRAM_NAME "background", nullptr));

    gNetworkActionsLooper.queue = dequeCreate(free);
    assert(gNetworkActionsLooper.thread = SDL_CreateThread(networkActionsLoop, PROGRAM_NAME "network", nullptr));

    networkInit();
}

bool lifecycleInitialized() {
    return gInitialized;
}

static void delayThread(const unsigned startMillis) {
    const unsigned differenceMillis = (unsigned) SDL_GetTicks() - startMillis;
    if (UPDATE_PERIOD > differenceMillis)
        SDL_Delay(UPDATE_PERIOD - differenceMillis);
}

static AsyncAction* nullable nextAsyncAction(const Looper looper) {
    return dequeQueuePop(looper == LOOPER_BACKGROUND ? gBackgroundActionsLooper.queue : gMainActionsLooper.queue, false);
}

static void threadLoop(void (* nullable const body)(), const Looper looper) {
    while (!gRunning) {} // wait for main thread to start looping
    unsigned startMillis;

    while (gRunning) {
        assert(startMillis = (unsigned) SDL_GetTicks());

        AsyncAction* action;
        if ((action = nextAsyncAction(looper))) {
            if (action->delayMillis) SDL_Delay(action->delayMillis);
            if (gRunning) action->function(action->parameter);
            free(action);
        }

        if (body) body();
        delayThread(startMillis);
    }
}

static int backgroundActionsLoop(void* nullable const) {
    threadLoop(nullptr, LOOPER_BACKGROUND);
    return 0;
}

static int networkActionsLoop(void* nullable const) {
    threadLoop(networkLoopBody, LOOPER_NETWORK);
    return 0;
}

static void scheduleAction(const LifecycleAsyncActionFunction function, void* nullable const parameter, const int delayMillis, const Looper looper) {
    assert(gInitialized);

    AsyncAction* const action = malloc(sizeof *action);
    assignToStructWithConsts(action, function, parameter, delayMillis)

    dequeQueuePush(looper == LOOPER_BACKGROUND ? gBackgroundActionsLooper.queue : gMainActionsLooper.queue, action);
}

void lifecycleRunInBackground(const LifecycleAsyncActionFunction function, void* nullable const parameter, const int delayMillis) {
    scheduleAction(function, parameter, delayMillis, LOOPER_BACKGROUND);
}

void lifecycleRunInMainThread(const LifecycleAsyncActionFunction function, void* nullable const parameter) {
    scheduleAction(function, parameter, 0, LOOPER_MAIN);
}

static void pulse() {
    char symbol = ' '; // - \ | / -...
    switch (gTicker) {
        case 0: symbol = '-'; gTicker = 1; break;
        case 1: symbol = '\\'; gTicker = 2; break;
        case 2: symbol = '|'; gTicker = 3; break;
        case 3: symbol = '/'; gTicker = 0; break;
        default: assert(false);
    }
    printf("\rRunning %c", symbol);
    fflush(stdout);
}

void lifecycleLoop() {
    assert(gInitialized);
    gRunning = true;

    unsigned startMillis;
    AsyncAction* action;

    while (true) {
        assert(startMillis = (unsigned) SDL_GetTicks());

        SDL_Event event;
        while (SDL_PollEvent(&event) == 1) {
            if (event.type == SDL_EVENT_QUIT)
                goto end;

        }

        if ((action = nextAsyncAction(LOOPER_MAIN))) {
            action->function(action->parameter);
            free(action);
        }

        delayThread(startMillis);
        pulse();
    }
    end:

    gRunning = false;
}

void lifecycleQuit() {
    assert(gInitialized);

    networkQuit();

    SDL_WaitThread(gNetworkActionsLooper.thread, nullptr);
    SDL_WaitThread(gBackgroundActionsLooper.thread, nullptr);

    gInitialized = false;

    dequeDestroy(gNetworkActionsLooper.queue);
    dequeDestroy(gBackgroundActionsLooper.queue);
    dequeDestroy(gMainActionsLooper.queue);

    SDL_Quit();
}
