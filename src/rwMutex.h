
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
void rwMutexReadLock(RWMutex* const rwMutex); // (1) can be locked twice or 2n times by the same thread as the underline implementation uses recursive mutex
void rwMutexReadUnlock(RWMutex* const rwMutex); // (2) must be unlocked 2n times if was locked by the same thread
void rwMutexWriteLock(RWMutex* const rwMutex); // same (1)
void rwMutexWriteUnlock(RWMutex* const rwMutex); // same (2)
bool rwMutexLocked(RWMutex* const rwMutex);
void rwMutexCommand(RWMutex* const rwMutex, const RWMutexCommand command);
void rwMutexDestroy(RWMutex* const rwMutex); // fails if the rwMutex is locked (either write or read)

typedef atomic bool Barrier;
#define BARRIER(x) Barrier x = false

bool barrierScopeBegin(Barrier* const barrier);
void barrierScopeEnd(Barrier* const barrier);
void barrierWait(Barrier* const barrier);
