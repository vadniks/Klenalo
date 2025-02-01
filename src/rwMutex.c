
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_atomic.h>
#include "defs.h"
#include "rwMutex.h"

// Inspired by https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock#Using_two_mutexes

struct _RWMutex {
    SDL_mutex* mutex;
    SDL_atomic_t readers;
    SDL_atomic_t writers;
};

RWMutex* rwMutexCreate(void) {
    RWMutex* const rwMutex = SDL_malloc(sizeof *rwMutex);
    assert(rwMutex);
    assert(rwMutex->mutex = SDL_CreateMutex());
    rwMutex->readers.value = 0;
    rwMutex->writers.value = 0;
    return rwMutex;
}

static inline int incrementCounter(SDL_atomic_t* const counter) {
    return SDL_AtomicAdd(counter, 1) + 1;
}

static inline int decrementCounter(SDL_atomic_t* const counter) {
    return SDL_AtomicAdd(counter, -1) - 1;
}

void rwMutexReadLock(RWMutex* const rwMutex) {
    if (incrementCounter(&rwMutex->readers))
        assert(!SDL_LockMutex(rwMutex->mutex));
}

void rwMutexReadUnlock(RWMutex* const rwMutex) {
    assert(SDL_AtomicGet(&rwMutex->readers));
    if (!decrementCounter(&rwMutex->readers))
        assert(!SDL_UnlockMutex(rwMutex->mutex));
}

void rwMutexWriteLock(RWMutex* const rwMutex) {
    assert(!SDL_LockMutex(rwMutex->mutex));
    incrementCounter(&rwMutex->writers);
}

void rwMutexWriteUnlock(RWMutex* const rwMutex) {
    assert(decrementCounter(&rwMutex->writers) >= 0);
    assert(!SDL_UnlockMutex(rwMutex->mutex));
}

bool rwMutexLocked(RWMutex* const rwMutex) {
    return SDL_AtomicGet(&rwMutex->writers) || SDL_AtomicGet(&rwMutex->readers);
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
    SDL_DestroyMutex(rwMutex->mutex);
    SDL_free(rwMutex);
}
