
#include "barrier.h"

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
