
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
void* listGet(const List* const list, const int index);
int listSize(const List* const list);
void* nullable listBinarySearch(const List* const list, const void* const key, const ListComparator comparator);
void listClear(List* const list);
void listDestroy(List* const list); // all values that are still remain inside a list at a time destroy is called are deallocated via supplied deallocator if it's not null
