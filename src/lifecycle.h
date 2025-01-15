
#pragma once

#include "defs.h"

typedef void (* LifecycleAsyncActionFunction)(void* nullable const);

void lifecycleInit(void);
bool lifecycleInitialized(void);
unsigned long lifecycleCurrentTimeMillis(void);
void lifecycleRunInBackground(const LifecycleAsyncActionFunction function, void* nullable const parameter, const int delayMillis);
void lifecycleRunInMainThread(const LifecycleAsyncActionFunction function, void* nullable const parameter);
void lifecycleAssertMainThread(void); // TODO <-------------------------------
void lifecycleLoop(void);
void lifecycleQuit(void);
