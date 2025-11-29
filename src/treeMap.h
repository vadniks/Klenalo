
#pragma once

#include "defs.h"

// Red-Black Tree (self-balancing binary search tree), optionally thread-safe, only works with non-null and unique values
// TODO: embed in the Hashtable, replacing the linked list

typedef void (* TreeMapDeallocator)(void* const);

typedef struct _TreeMap TreeMap;
typedef struct _TreeMapIterator TreeMapIterator;

TreeMap* treeMapCreate(const bool synchronized, const TreeMapDeallocator nullable deallocator);
int treeMapCount(TreeMap* const map);
int treeMapIteratorSize(TreeMap* const map);
void treeMapInsert(TreeMap* const map, const int key, void* const value);
void* nullable treeMapSearchKey(TreeMap* const map, const int key);
void* nullable treeMapSearchMinOrMax(TreeMap* const map, const bool minOrMax);
void treeMapDelete(TreeMap* const map, const int key);
void treeMapIterateBegin(TreeMap* const map, TreeMapIterator* const iterator);
#define treeMapIterateBegin(x, y) treeMapIterateBegin(x, (y = xalloca2(treeMapIteratorSize(x))))
void* nullable treeMapIterate(TreeMapIterator* const iterator); // assumed readonly - don't do any write operations while iterating, iteration involves mutex read-locking
void treeMapIterateEnd(TreeMapIterator* const iterator);
void treeMapDestroy(TreeMap* const map);
