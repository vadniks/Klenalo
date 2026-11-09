
#pragma once

#include "../defs.h"

// Doubly linked list, queue and stack, only works with non-null values

typedef struct _Deque Deque;

Deque* dequeCreate(Deallocator nullable deallocator); // if the deallocator isn't null then use it as free alternative, do nothing otherwise
void dequePushBack(Deque* deque, void* value);
#define dequeStackPush dequePushBack
void dequePushFront(Deque* deque, void* value);
#define dequeQueuePush dequePushFront
int dequeSize(const Deque* deque);
void* nullable dequePopFirst(Deque* deque, bool delete); // if delete then free the value and return null
#define dequeQueuePop dequePopFirst
void* nullable dequePopLast(Deque* deque, bool delete);
#define dequeStackPop dequePopLast
void* nullable dequePeekFirst(const Deque* deque);
#define dequeQueuePeek dequePeekFirst
void* nullable dequePeekLast(const Deque* deque);
#define dequeStackPeek dequePeekLast
void dequeClear(Deque* deque);
void dequeDestroy(Deque* deque);
