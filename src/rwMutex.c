
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_atomic.h>
#include "defs.h"
#include "rwMutex.h"

// https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock#Using_two_mutexes

struct _RWMutex {
    SDL_mutex* mutex;
    SDL_atomic_t counter;
};

RWMutex* rwMutexInit(void) {
    RWMutex* const rwMutex = SDL_malloc(sizeof *rwMutex);
    assert(rwMutex);
    assert(rwMutex->mutex = SDL_CreateMutex());
    rwMutex->counter.value = 0;
    return rwMutex;
}

static inline int incrementCounter(SDL_atomic_t* const counter) {
    return SDL_AtomicAdd(counter, 1) + 1;
}

static inline int decrementCounter(SDL_atomic_t* const counter) {
    return SDL_AtomicAdd(counter, -1) - 1;
}

void rwMutexReadLock(RWMutex* const rwMutex) {
    if (incrementCounter(&rwMutex->counter))
        assert(!SDL_LockMutex(rwMutex->mutex));
}

void rwMutexReadUnlock(RWMutex* const rwMutex) {
    if (!decrementCounter(&rwMutex->counter))
        assert(!SDL_UnlockMutex(rwMutex->mutex));
}

void rwMutexWriteLock(RWMutex* const rwMutex) {
    assert(!SDL_LockMutex(rwMutex->mutex));
}

void rwMutexWriteUnlock(RWMutex* const rwMutex) {
    assert(!SDL_UnlockMutex(rwMutex->mutex));
}

void rwMutexDestroy(RWMutex* const rwMutex) {
    SDL_DestroyMutex(rwMutex->mutex);
    SDL_free(rwMutex);
}
