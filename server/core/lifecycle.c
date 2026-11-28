// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#include <sanitizer/lsan_interface.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../common/crypto/crypto.h"
#include "../network/network.h"
#include "../../common/collections/deque.h"
#include "lifecycle.h"

typedef struct {
    const LifecycleAsyncActionFunction function;
    void* nullable const parameter;
} AsyncAction;

static constexpr int UPDATE_PERIOD = 1000 / 60; // floorf(1000.0f / 60.0f)

static bool gInitialized = false;
static bool gRunning = false;
static int gTicker = 0;

typed(Deque*, AsyncAction*) nullable gActionsQueue = nullptr;

static void signalHandler(const int signal) {
    assert(signal == SIGINT || signal == SIGTERM);
    gRunning = false;
}

void lifecycleInit() {
    assert(!getenv("LD_PRELOAD"));
    // printf("debugging: %s\n", boolToStr(ptrace(PTRACE_TRACEME, 0, nullptr, 0) < 0));
    setvbuf(stdout, nullptr, _IONBF, 0); // disable heap buffers for stdout

    assert(!gInitialized);
    gInitialized = true;

    assert(!signal(SIGINT, signalHandler));
    assert(!signal(SIGTERM, signalHandler));

    gActionsQueue = dequeCreate(free);

    cryptoInit();
    networkInit();
}

bool lifecycleInitialized() {
    return gInitialized;
}

static void delayThread(const unsigned startMillis) {
    const unsigned differenceMillis = (unsigned) currentTimeMillis() - startMillis;
    if (UPDATE_PERIOD > differenceMillis)
        xsleep(UPDATE_PERIOD - differenceMillis);
}

void lifecycleRunDeferred(const LifecycleAsyncActionFunction function, void* nullable const parameter) {
    assert(gInitialized);

    AsyncAction* const action = malloc(sizeof *action);
    assignToStructWithConsts(action, function, parameter)

    dequeQueuePush(gActionsQueue, action);
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

    while (gRunning) {
        assert(startMillis = (unsigned) currentTimeMillis());

        if ((action = dequeQueuePop(gActionsQueue, false))) {
            action->function(action->parameter);
            free(action);
        }

        networkLoopBody();

        delayThread(startMillis);
        // pulse();
    }

    gRunning = false;
}

void lifecycleQuit() {
    assert(gInitialized);
    gInitialized = false;

    networkQuit();
    cryptoQuit();

    dequeDestroy(gActionsQueue);
}
