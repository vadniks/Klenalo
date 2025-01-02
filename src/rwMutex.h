
#pragma once

#define RW_MUTEX_READ_LOCKED(m, a) rwMutexReadLock(m); a rwMutexReadUnlock(m);
#define RW_MUTEX_WRITE_LOCKED(m, a) rwMutexWriteLock(m); a rwMutexWriteUnlock(m);

struct _RWMutex;
typedef struct _RWMutex RWMutex;

RWMutex* rwMutexInit(void);
void rwMutexReadLock(RWMutex* const rwMutex);
void rwMutexReadUnlock(RWMutex* const rwMutex);
void rwMutexWriteLock(RWMutex* const rwMutex);
void rwMutexWriteUnlock(RWMutex* const rwMutex);
void rwMutexDestroy(RWMutex* const rwMutex);
