
// TODO: rollback to previous implementation for a recursive rw lock

#include <SDL3/SDL_mutex.h>
#include "rwMutex.h"

struct _RWMutex {
    SDL_RWLock* const lock;
    atomic int readers;
    atomic int writers;
};

RWMutex* rwMutexCreate(void) {
    RWMutex* const rwMutex = xmalloc(sizeof *rwMutex);
    assert(rwMutex);
    assert(unconst(rwMutex->lock) = SDL_CreateRWLock());
    rwMutex->readers = 0;
    rwMutex->writers = 0;
    return rwMutex;
}

void rwMutexReadLock(RWMutex* const rwMutex) {
    SDL_LockRWLockForReading(rwMutex->lock);
    rwMutex->readers++;
}

void rwMutexReadUnlock(RWMutex* const rwMutex) {
    assert(rwMutex->readers-- > 0);
    SDL_UnlockRWLock(rwMutex->lock);
}

void rwMutexWriteLock(RWMutex* const rwMutex) {
    SDL_LockRWLockForWriting(rwMutex->lock);
    rwMutex->writers++;
}

void rwMutexWriteUnlock(RWMutex* const rwMutex) {
    assert(rwMutex->writers-- > 0);
    SDL_UnlockRWLock(rwMutex->lock);
}

bool rwMutexLocked(RWMutex* const rwMutex) {
    return rwMutex->readers > 0 || rwMutex->writers > 0;
}

void rwMutexCommand(RWMutex* const rwMutex, const RWMutexCommand command) {
    switch (command) {
        case RW_MUTEX_COMMAND_READ_LOCK: rwMutexReadLock(rwMutex); break;
        case RW_MUTEX_COMMAND_READ_UNLOCK: rwMutexReadUnlock(rwMutex); break;
        case RW_MUTEX_COMMAND_WRITE_LOCK: rwMutexWriteLock(rwMutex); break;
        case RW_MUTEX_COMMAND_WRITE_UNLOCK: rwMutexWriteUnlock(rwMutex); break;
    }
}

void rwMutexDestroy(RWMutex* const rwMutex) {
    assert(!rwMutexLocked(rwMutex));
    SDL_DestroyRWLock(rwMutex->lock);
    xfree(rwMutex);
}
