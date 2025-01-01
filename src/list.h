
#pragma once

#include "defs.h"

struct _List;
typedef struct _List List;

typedef void (* ListDeallocator)(void* const);
typedef int (* ListComparator)(const void* const, const void* const);
typedef void* (* ListItemDuplicator)(const void* const);

List* listInit(const ListDeallocator nullable deallocator);
List* listCopy(const List* const old, const ListItemDuplicator itemDuplicator);
void listAddBack(List* const list, void* const value);
void listAddFront(List* const list, void* const value);
void listPush(List* const list, void* const value);
void* listGet(const List* const list, const int index);
void* listPopFirst(List* const list);
void* listPopLast(List* const list);
void listRemove(List* const list, const int index);
void* nullable listPeekFirst(const List* const list);
void* nullable listPeekLast(const List* const list);
int listSize(const List* const list);
void* nullable listBinarySearch(const List* const list, const void* const key, const ListComparator comparator);
void listClear(List* const list);
void listDestroy(List* const list);
