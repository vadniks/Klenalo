
#pragma once

#include "../../common/defs.h"

typedef void (* LifecycleAsyncActionFunction)(void* nullable);

void lifecycleInit();
bool lifecycleInitialized();
void lifecycleRunInBackground(LifecycleAsyncActionFunction function, void* nullable parameter, int delayMillis);
void lifecycleRunInMainThread(LifecycleAsyncActionFunction function, void* nullable parameter);
void lifecycleLoop();
void lifecycleQuit();
