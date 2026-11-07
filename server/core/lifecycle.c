
#include <SDL3/SDL.h>
#include "../../common/collections/vector.h"
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

static constexpr int UPDATE_PERIOD = 16; // floorf(1000.0f / 60.0f)

static atomic bool gInitialized = false;
static atomic bool gRunning = false;

static struct {
    Vector* nullable queue; // <AsyncAction*>
    SDL_Thread* nullable thread;
}
    gMainActionsLooper = {nullptr, nullptr},
    gBackgroundActionsLooper = {nullptr, nullptr};

static unsigned getTicks();
static int backgroundActionsLoop(void* nullable);

void lifecycleInit() {
    assert(!gInitialized);
    gInitialized = true;

    assert(SDL_Init(SDL_INIT_EVENTS));

    gMainActionsLooper.queue = vectorCreate(free);
    gBackgroundActionsLooper.queue = vectorCreate(free);

    assert(gBackgroundActionsLooper.thread = SDL_CreateThread(backgroundActionsLoop, PROGRAM_NAME "background", nullptr));
}

static unsigned getTicks() {
    return (unsigned) SDL_GetTicks();
}

static void delayThread(const unsigned startMillis) {
    const unsigned differenceMillis = (unsigned) SDL_GetTicks() - startMillis;
    if (UPDATE_PERIOD > differenceMillis)
        SDL_Delay(UPDATE_PERIOD - differenceMillis);
}

static AsyncAction* nullable nextAsyncAction(const Looper looper) {
    return vectorPopFirst(looper == LOOPER_BACKGROUND ? gBackgroundActionsLooper.queue : gMainActionsLooper.queue);
}

static void threadLoop(void (* const body)()) {
    while (!gRunning) {} // wait for main thread to start looping

    unsigned startMillis;

    while (gRunning) {
        assert(startMillis = (unsigned) SDL_GetTicks());
        body();
        delayThread(startMillis);
    }
}

static void backgroundActionsLoopBody() {
    AsyncAction* action;
    if ((action = nextAsyncAction(LOOPER_BACKGROUND))) {
        if (action->delayMillis) SDL_Delay(action->delayMillis);
        if (gRunning) action->function(action->parameter);
        free(action);
    }
}

static int backgroundActionsLoop(void* nullable const) {
    threadLoop(backgroundActionsLoopBody);
    return 0;
}

static void scheduleAction(const LifecycleAsyncActionFunction function, void* nullable const parameter, const int delayMillis, const Looper looper) {
    assert(gInitialized);

    AsyncAction* const action = malloc(sizeof *action);
    assignToStructWithConsts(action, function, parameter, delayMillis)

    vectorAddFront(looper == LOOPER_BACKGROUND ? gBackgroundActionsLooper.queue : gMainActionsLooper.queue, action);
}

void lifecycleRunInBackground(const LifecycleAsyncActionFunction function, void* nullable const parameter, const int delayMillis) {
    scheduleAction(function, parameter, delayMillis, LOOPER_BACKGROUND);
}

void lifecycleRunInMainThread(const LifecycleAsyncActionFunction function, void* nullable const parameter) {
    scheduleAction(function, parameter, 0, LOOPER_MAIN);
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
    }
    end:

    gRunning = false;
}

void lifecycleQuit() {
    assert(gInitialized);

    SDL_WaitThread(gBackgroundActionsLooper.thread, nullptr);

    gInitialized = false;

    vectorDestroy(gBackgroundActionsLooper.queue);
    vectorDestroy(gMainActionsLooper.queue);

    SDL_Quit();
}
