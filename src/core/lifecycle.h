
#pragma once

#include "../defs.h"

typedef void (* LifecycleAsyncActionFunction)(void* nullable);

void lifecycleInit();
void lifecycleRunInBackground(LifecycleAsyncActionFunction function, void* nullable parameter, int delayMillis);
void lifecycleRunInMainThread(LifecycleAsyncActionFunction function, void* nullable parameter);
void lifecycleLoop();
void lifecycleQuit();
