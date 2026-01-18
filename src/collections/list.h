
#pragma once

#include "../defs.h"

// Linear list (dynamically resizable array - aka vector), queue and stack operations support [deprecated], optionally thread-safe, only works with non-null values

// TODO: rename to vector?

// TODO: add bulk remove - deletion of several items at once

typedef struct _List List;

List* listCreate(const Allocator* const internalAllocator, const bool synchronized, const Deallocator nullable deallocator);
List* nullable listCopy(List* const old, const bool synchronized, const Duplicator nullable duplicator);
void listAddBack(List* const list, void* const value); // listAdd, stackPush
void listAddFront(List* const list, void* const value); // queuePush
void* nullable listGet(List* const list, const int index);
void listSwap(List* const list, const int index1, const int index2); // swap places items with corresponding indexes - first goes to second's position and v.v.
void* nullable listPopFirst(List* const list); // queuePop // TODO: rename to remove(First|Last)
void* nullable listPopLast(List* const list); // stackPop
void listRemove(List* const list, const int index); // don't use in a loop - ConcurrentModificationException - or each iteration adjust loop counter
void* nullable listPeekFirst(List* const list); // queuePeek
void* nullable listPeekLast(List* const list); // stackPeek
int listSize(List* const list);
void* nullable listBinarySearch(List* const list, const void* const key, const Comparator comparator);
void listQSort(List* const list, const Comparator comparator);
void listClear(List* const list);
void listDestroy(List* const list);
