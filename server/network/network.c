// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#include "../core/lifecycle.h"
#include "discovery.h"
#include "network.h"

static atomic bool gInitialized = false;

static void queryInterfaces0();

void networkInit() {
    assert(!gInitialized && lifecycleInitialized());
    gInitialized = true;

    assert(NET_Init());

    discoveryInit();
}

bool networkInitialized() {
    return gInitialized;
}

void networkLoopBody() {
    discoveryListen();
    discoveryPing();
}

void networkQuit() {
    assert(gInitialized);
    gInitialized = false;

    discoveryQuit();
    NET_Quit();
}
