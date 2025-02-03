
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_mutex.h>
#include <SDL2/SDL_atomic.h>
#include "defs.h"
#include "rwMutex.h"

// Inspired by https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock#Using_two_mutexes

struct _RWMutex {
    SDL_mutex* mutex; // const
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
    const int readers = decrementCounter(&rwMutex->readers);
    assert(readers >= 0);
    if (!readers)
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

bool barrierScopeBegin(Barrier* const barrier) {
    if (*barrier) return true;
    *barrier = true;
    return false;
}

void barrierScopeEnd(Barrier* const barrier) {
    *barrier = false;
}

void barrierWait(Barrier* const barrier) {
    while (*barrier) asm volatile ("call thrd_yield");
    *barrier = true;
}

struct _ConditionObserver {
    void* const variablePointer; // allocated elsewhere
    const ConditionObserverVariableType variableType;
    SDL_mutex* const mutex;
    SDL_cond* const cond;
    bool locked;
};

ConditionObserver* conditionObserverCreate(void* const variablePointer, const ConditionObserverVariableType variableType) {
    ConditionObserver* const observer = SDL_malloc(sizeof *observer);
    *(void**) &observer->variablePointer = variablePointer;
    *(int*) &observer->variableType = variableType;
    assert(*(SDL_mutex**) &observer->mutex = SDL_CreateMutex());
    assert(*(SDL_cond**) &observer->cond = SDL_CreateCond());
    observer->locked = false;
    return observer;
}

void conditionObserverGetVariableValue(ConditionObserver* const observer, void* const buffer) {
    assert(!SDL_LockMutex(observer->mutex));
    SDL_memcpy(buffer, observer->variablePointer, observer->variableType);
    assert(!SDL_UnlockMutex(observer->mutex));
}

void conditionObserverSetVariableValue(ConditionObserver* const observer, const void* const value) {
    assert(!SDL_LockMutex(observer->mutex));
    observer->locked = true;

    SDL_memcpy(observer->variablePointer, value, observer->variableType);
    assert(!SDL_CondBroadcast(observer->cond));

    observer->locked = false;
    assert(!SDL_UnlockMutex(observer->mutex));
}

void conditionObserverWaitForVariableValue(ConditionObserver* const observer, const void* const value) {
    assert(!SDL_LockMutex(observer->mutex));
    observer->locked = true;

    while (SDL_memcmp(observer->variablePointer, value, observer->variableType) != 0)
        assert(!SDL_CondWait(observer->cond, observer->mutex));

    observer->locked = false;
    assert(!SDL_UnlockMutex(observer->mutex));
}

void conditionObserverDestroy(ConditionObserver* const observer) {
    assert(!SDL_LockMutex(observer->mutex));
    assert(!observer->locked);
    assert(!SDL_UnlockMutex(observer->mutex));

    SDL_DestroyCond(observer->cond);
    SDL_DestroyMutex(observer->mutex);
    SDL_free(observer);
}
