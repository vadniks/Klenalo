
#pragma once

#include "../defs.h"

// Doubly linked list, queue and stack, optionally thread-safe, only works with non-null values

typedef struct _Deque Deque;

Deque* dequeCreate(const bool synchronized, const Deallocator deallocator);
void dequePushBack(Deque* const deque, void* const value); // vectorAdd, stackPush
void dequePushFront(Deque* const deque, void* const value); // queuePush
void* nullable dequeGet(Deque* const deque, const int index, const bool fromStartOrFromEnd);
int dequeSize(Deque* const deque);
void* nullable dequePopFirst(Deque* const deque); // retrieve just removed item, queuePop
void* nullable dequePopLast(Deque* const deque); // retrieve just removed item, stackPop
void dequeRemove(Deque* const deque, const int index);
void* nullable dequePeekFirst(Deque* const deque); // queuePeek
void* nullable dequePeekLast(Deque* const deque); // stackPeek
void dequeClear(Deque* const deque);
void dequeDestroy(Deque* const deque);
