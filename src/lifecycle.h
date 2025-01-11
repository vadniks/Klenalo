
#pragma once

#include "defs.h"

typedef void (* LifecycleAsyncActionFunction)(void* nullable const);

void lifecycleInit(void);
bool lifecycleInitialized(void);
void lifecycleAsync(const LifecycleAsyncActionFunction function, void* nullable const parameter, int delayMillis);
void lifecycleLoop(void);
void lifecycleQuit(void);
