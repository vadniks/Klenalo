
#pragma once

#include "defs.h"

struct _RWMutex;
typedef struct _RWMutex RWMutex;

typedef enum {
    RW_MUTEX_COMMAND_READ_LOCK,
    RW_MUTEX_COMMAND_READ_UNLOCK,
    RW_MUTEX_COMMAND_WRITE_LOCK,
    RW_MUTEX_COMMAND_WRITE_UNLOCK
} RWMutexCommand;

RWMutex* rwMutexCreate(void);
void rwMutexReadLock(RWMutex* const rwMutex); // (1) can be locked twice or 2n times by the same thread as the implementation uses recursive mutex
void rwMutexReadUnlock(RWMutex* const rwMutex); // (2) must be unlocked 2n times if was locked by the same thread
void rwMutexWriteLock(RWMutex* const rwMutex); // same (1)
void rwMutexWriteUnlock(RWMutex* const rwMutex); // same (2)
bool rwMutexLocked(RWMutex* const rwMutex);
void rwMutexCommand(RWMutex* const rwMutex, const RWMutexCommand command);
void rwMutexDestroy(RWMutex* const rwMutex); // fails if the rwMutex is locked (either write or read)

// TODO: move to separate module
typedef atomic bool Barrier; // multiple threads can wait while one another thread is running or looping or running a next iteration of the loop - basically wait for the thread to finish when SDL_WaitThread or similar cannot be used
#define BARRIER(x) Barrier x = false

bool barrierScopeBegin(Barrier* const barrier); // returns true if current thread loop iteration must be skipped (or even the whole loop must be stopped - depends on the case)
void barrierScopeEnd(Barrier* const barrier);
inline void barrierReset(Barrier* const barrier) { barrierScopeEnd(barrier); }
void barrierWait(Barrier* const barrier);

// TODO: move to separate module
typedef enum : byte {
    CONDITION_OBSERVER_VARIABLE_TYPE_BYTE = sizeof(byte),
    CONDITION_OBSERVER_VARIABLE_TYPE_CHAR = CONDITION_OBSERVER_VARIABLE_TYPE_BYTE,
    CONDITION_OBSERVER_VARIABLE_TYPE_BOOL = CONDITION_OBSERVER_VARIABLE_TYPE_BYTE,
    CONDITION_OBSERVER_VARIABLE_TYPE_SHORT = sizeof(short),
    CONDITION_OBSERVER_VARIABLE_TYPE_INT = sizeof(int),
    CONDITION_OBSERVER_VARIABLE_TYPE_LONG = sizeof(long),
} ConditionObserverVariableType;

struct _ConditionObserver;
typedef struct _ConditionObserver ConditionObserver;

ConditionObserver* conditionObserverCreate(void* const variablePointer, const ConditionObserverVariableType variableType);
void conditionObserverSetVariableValue(ConditionObserver* const observer, const void* const value);
void conditionObserverWaitForVariableValue(ConditionObserver* const observer, const void* const value);
void conditionObserverDestroy(ConditionObserver* const observer);
