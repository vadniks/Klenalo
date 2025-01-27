
#pragma once

#include "defs.h"

// Linear list, queue and stack, optionally thread-safe, not intended to store nulls

typedef void (* ListDeallocator)(void* const);
typedef int (* ListComparator)(const void* const, const void* const);
typedef void* (* ListItemDuplicator)(const void* const);

struct _List;
typedef struct _List List;

List* listCreate(const bool synchronized, const ListDeallocator nullable deallocator);
List* listCopy(List* const old, const bool synchronized, const ListItemDuplicator itemDuplicator);
void listAddBack(List* const list, void* const value); // listAdd, stackPush
void listAddFront(List* const list, void* const value); // queuePush
void* listGet(List* const list, const int index);
void* listPopFirst(List* const list); // queuePop
void* listPopLast(List* const list); // stackPop
void listRemove(List* const list, const int index);
void* nullable listPeekFirst(List* const list); // queuePeek
void* nullable listPeekLast(List* const list); // stackPeek
int listSize(List* const list);
void* nullable listBinarySearch(List* const list, const void* const key, const ListComparator comparator);
void listQSort(List* const list, const ListComparator comparator);
void listClear(List* const list);
void listDestroy(List* const list);

#if TESTING
void listRunTests(void);
#endif
