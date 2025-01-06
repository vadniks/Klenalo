
#pragma once

#include "defs.h"

// Linear list, queue and stack, not thread-safe, allows null values

typedef void (* ListDeallocator)(void* const);
typedef int (* ListComparator)(const void* const, const void* const);
typedef void* (* ListItemDuplicator)(const void* const);

struct _List;
typedef struct _List List;

List* listInit(const ListDeallocator nullable deallocator);
List* listCopy(const List* const old, const ListItemDuplicator itemDuplicator);
void listAddBack(List* const list, void* const value); // listAdd, stackPush
void listAddFront(List* const list, void* const value); // queuePush
void* listGet(const List* const list, const int index);
void* listPopFirst(List* const list); // queuePop
void* listPopLast(List* const list); // stackPop
void listRemove(List* const list, const int index);
void* nullable listPeekFirst(const List* const list); // queuePeek
void* nullable listPeekLast(const List* const list); // stackPeek
int listSize(const List* const list);
void* nullable listBinarySearch(const List* const list, const void* const key, const ListComparator comparator);
void listQSort(List* const list, const ListComparator comparator);
void listClear(List* const list);
void listDestroy(List* const list);

#if TESTING
void listRunTests(void);
#endif
