
#pragma once

#include "defs.h"

typedef void (* LifecycleAsyncActionFunction)(void* nullable const);

void lifecycleInit(void);
bool lifecycleInitialized(void);
unsigned long lifecycleCurrentTimeMillis(void);
void lifecycleAsync(const LifecycleAsyncActionFunction function, void* nullable const parameter, const int delayMillis); // thread-safe
void lifecycleLoop(void);
void lifecycleQuit(void);
