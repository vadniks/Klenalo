
#include <SDL3/SDL_mutex.h>
#include "conditionObserver.h"

struct _ConditionObserver {
    void* const variablePointer; // allocated elsewhere
    const ConditionObserverVariableType variableType;
    SDL_Mutex* const mutex;
    SDL_Condition* const condition;
    bool locked;
};

ConditionObserver* conditionObserverCreate(void* const variablePointer, const ConditionObserverVariableType variableType) {
    ConditionObserver* const observer = xmalloc(sizeof *observer);
    unconst(observer->variablePointer) = variablePointer;
    unconst(observer->variableType) = variableType;
    assert(unconst(observer->mutex) = SDL_CreateMutex());
    assert(unconst(observer->condition) = SDL_CreateCondition());
    observer->locked = false;
    return observer;
}

void conditionObserverGetVariableValue(ConditionObserver* const observer, void* const buffer) {
    SDL_LockMutex(observer->mutex);
    xmemcpy(buffer, observer->variablePointer, observer->variableType);
    SDL_UnlockMutex(observer->mutex);
}

void conditionObserverSetVariableValue(ConditionObserver* const observer, const void* const value) {
    SDL_LockMutex(observer->mutex);
    observer->locked = true;

    xmemcpy(observer->variablePointer, value, observer->variableType);
    SDL_BroadcastCondition(observer->condition);

    observer->locked = false;
    SDL_UnlockMutex(observer->mutex);
}

void conditionObserverWaitForVariableValue(ConditionObserver* const observer, const void* const value) {
    SDL_LockMutex(observer->mutex);
    observer->locked = true;

    while (xmemcmp(observer->variablePointer, value, observer->variableType) != 0)
        SDL_WaitCondition(observer->condition, observer->mutex);

    observer->locked = false;
    SDL_UnlockMutex(observer->mutex);
}

void conditionObserverDestroy(ConditionObserver* const observer) {
    SDL_LockMutex(observer->mutex);
    assert(!observer->locked);
    SDL_UnlockMutex(observer->mutex);

    SDL_DestroyCondition(observer->condition);
    SDL_DestroyMutex(observer->mutex);
    xfree(observer);
}
