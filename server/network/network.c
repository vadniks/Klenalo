// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#include "../core/lifecycle.h"
#include "discovery.h"
#include "network.h"

enum {
    LISTEN_PERIOD = 1000,
    PING_PERIOD = 1000
};

static atomic bool gInitialized = false;

void networkInit() {
    assert(!gInitialized && lifecycleInitialized());
    gInitialized = true;

    discoveryInit();
}

bool networkInitialized() {
    return gInitialized;
}

void networkLoopBody() {
    static ulong lastListen = 0, lastPing = 0;
    runPeriodically(&lastListen, LISTEN_PERIOD, discoveryListen);
    runPeriodically(&lastPing, PING_PERIOD, discoveryPing);
}

void networkQuit() {
    assert(gInitialized);
    gInitialized = false;

    discoveryQuit();
}
