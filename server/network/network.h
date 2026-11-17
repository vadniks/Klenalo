
#pragma once

#include "shared.h"

void networkInit();
bool networkInitialized();
bool networkIsLoopback(const In6Addr* addr);
void networkLoopBody();
void networkQuit();
