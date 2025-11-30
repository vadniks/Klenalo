
#pragma once

#include "../defs.h"

// Linear list (dynamically resizable array - aka vector), queue and stack operations support [deprecated], optionally thread-safe, only works with non-null values

typedef struct _List List;

List* listCreate(const bool synchronized, const Deallocator nullable deallocator);
List* nullable listCopy(List* const old, const bool synchronized, const ValueDuplicator nullable duplicator);
void listAddBack(List* const list, void* const value); // listAdd, stackPush
void listAddFront(List* const list, void* const value); // queuePush
void* nullable listGet(List* const list, const int index);
void* nullable listPopFirst(List* const list); // queuePop
void* nullable listPopLast(List* const list); // stackPop
void listRemove(List* const list, const int index);
void* nullable listPeekFirst(List* const list); // queuePeek
void* nullable listPeekLast(List* const list); // stackPeek
int listSize(List* const list);
void* nullable listBinarySearch(List* const list, const void* const key, const Comparator comparator);
void listQSort(List* const list, const Comparator comparator);
void listClear(List* const list);
void listDestroy(List* const list);
