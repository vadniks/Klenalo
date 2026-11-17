// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "shared.h"

void networkInit();
bool networkInitialized();
bool networkIsLoopback(const In6Addr* addr);
void networkLoopBody();
void networkQuit();
