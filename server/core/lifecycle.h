// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include "../../common/defs.h"

typedef void (* LifecycleAsyncActionFunction)(void* nullable);

void lifecycleInit();
bool lifecycleInitialized();
void lifecycleRunDeferred(LifecycleAsyncActionFunction function, void* nullable parameter);
void lifecycleLoop();
void lifecycleQuit();
