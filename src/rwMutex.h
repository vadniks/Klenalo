
#pragma once

#define RW_MUTEX_READ_LOCKED(m, a) rwMutexReadLock(m); a rwMutexReadUnlock(m);
#define RW_MUTEX_WRITE_LOCKED(m, a) rwMutexWriteLock(m); a rwMutexWriteUnlock(m);

struct _RWMutex;
typedef struct _RWMutex RWMutex;

typedef enum {
    RW_MUTEX_COMMAND_READ_LOCK,
    RW_MUTEX_COMMAND_READ_UNLOCK,
    RW_MUTEX_COMMAND_WRITE_LOCK,
    RW_MUTEX_COMMAND_WRITE_UNLOCK
} RWMutexCommand;

RWMutex* rwMutexCreate(void);
void rwMutexReadLock(RWMutex* const rwMutex);
void rwMutexReadUnlock(RWMutex* const rwMutex);
void rwMutexWriteLock(RWMutex* const rwMutex);
void rwMutexWriteUnlock(RWMutex* const rwMutex);
bool rwMutexLocked(RWMutex* const rwMutex);
void rwMutexCommand(RWMutex* const rwMutex, const RWMutexCommand command);
void rwMutexDestroy(RWMutex* const rwMutex); // fails if the rwMutex is locked (either write or read)
