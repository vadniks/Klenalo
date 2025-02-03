
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_mutex.h>
#include "conditionObserver.h"

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
