
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
    ConditionObserver* const observer = xmalloc(sizeof *observer);
    *(void**) &observer->variablePointer = variablePointer;
    *(int*) &observer->variableType = variableType;
    assert(*(SDL_mutex**) &observer->mutex = SDL_CreateMutex());
    assert(*(SDL_cond**) &observer->cond = SDL_CreateCond());
    observer->locked = false;
    return observer;
}

void conditionObserverGetVariableValue(ConditionObserver* const observer, void* const buffer) {
    assert(!SDL_LockMutex(observer->mutex));
    xmemcpy(buffer, observer->variablePointer, observer->variableType);
    assert(!SDL_UnlockMutex(observer->mutex));
}

void conditionObserverSetVariableValue(ConditionObserver* const observer, const void* const value) {
    assert(!SDL_LockMutex(observer->mutex));
    observer->locked = true;

    xmemcpy(observer->variablePointer, value, observer->variableType);
    assert(!SDL_CondBroadcast(observer->cond));

    observer->locked = false;
    assert(!SDL_UnlockMutex(observer->mutex));
}

void conditionObserverWaitForVariableValue(ConditionObserver* const observer, const void* const value) {
    assert(!SDL_LockMutex(observer->mutex));
    observer->locked = true;

    while (xmemcmp(observer->variablePointer, value, observer->variableType) != 0)
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
    xfree(observer);
}
