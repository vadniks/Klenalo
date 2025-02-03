
#pragma once

#include "defs.h"

typedef atomic bool Barrier; // multiple threads can wait while one another thread is running or looping or running a next iteration of the loop - basically wait for the thread to finish when SDL_WaitThread or similar cannot be used
#define BARRIER(x) Barrier x = false

bool barrierScopeBegin(Barrier* const barrier); // returns true if current thread loop iteration must be skipped (or even the whole loop must be stopped - depends on the case)
void barrierScopeEnd(Barrier* const barrier);
inline void barrierReset(Barrier* const barrier) { barrierScopeEnd(barrier); }
void barrierWait(Barrier* const barrier);
