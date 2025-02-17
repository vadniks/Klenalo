
#include <SDL2/SDL_mutex.h>
#include "rwMutex.h"

// Inspired by https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock#Using_two_mutexes

struct _RWMutex {
    SDL_mutex* const mainMutex;
    SDL_mutex* const auxiliaryMutex;
    int readers;
    int writers;
};

RWMutex* rwMutexCreate(void) {
    RWMutex* const rwMutex = xmalloc(sizeof *rwMutex);
    assert(rwMutex);
    assert(*(SDL_mutex**) &rwMutex->mainMutex = SDL_CreateMutex());
    assert(*(SDL_mutex**) &rwMutex->auxiliaryMutex = SDL_CreateMutex());
    rwMutex->readers = 0;
    rwMutex->writers = 0;
    return rwMutex;
}

void rwMutexReadLock(RWMutex* const rwMutex) {
    assert(!SDL_LockMutex(rwMutex->auxiliaryMutex));

    if (++rwMutex->readers == 1)
        assert(!SDL_LockMutex(rwMutex->mainMutex));

    assert(!SDL_UnlockMutex(rwMutex->auxiliaryMutex));
}

void rwMutexReadUnlock(RWMutex* const rwMutex) {
    assert(!SDL_LockMutex(rwMutex->auxiliaryMutex));

    assert(rwMutex->readers);
    if (--rwMutex->readers == 0)
        assert(!SDL_UnlockMutex(rwMutex->mainMutex));

    assert(!SDL_UnlockMutex(rwMutex->auxiliaryMutex));
}

void rwMutexWriteLock(RWMutex* const rwMutex) {
    assert(!SDL_LockMutex(rwMutex->mainMutex));
    rwMutex->writers++;
}

void rwMutexWriteUnlock(RWMutex* const rwMutex) {
    assert(--rwMutex->writers >= 0);
    assert(!SDL_UnlockMutex(rwMutex->mainMutex));
}

bool rwMutexLocked(RWMutex* const rwMutex) {
    assert(!SDL_LockMutex(rwMutex->mainMutex));
    const bool writers = rwMutex->writers;
    assert(!SDL_UnlockMutex(rwMutex->mainMutex));

    assert(!SDL_LockMutex(rwMutex->auxiliaryMutex));
    const bool readers = rwMutex->readers;
    assert(!SDL_UnlockMutex(rwMutex->auxiliaryMutex));

    return writers || readers;
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
    SDL_DestroyMutex(rwMutex->mainMutex);
    SDL_DestroyMutex(rwMutex->auxiliaryMutex);
    xfree(rwMutex);
}
