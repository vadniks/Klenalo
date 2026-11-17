
#pragma once

#include "shared.h"

void networkInit();
bool networkInitialized();
bool networkIsLoopback(const In6Addr* addr);
void networkRunPeriodically(
    ulong* lastRunMillis,
    int periodMillis,
    void (* action)(void)
);
void networkLoopBody();
void networkQuit();
