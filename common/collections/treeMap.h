
#pragma once

#include "../defs.h"

// Red-Black Tree (self-balancing binary search tree), only works with non-null and unique values

typedef struct _TreeMap TreeMap;
typedef struct _TreeMapIterator TreeMapIterator;

TreeMap* treeMapCreate(Deallocator nullable deallocator);
int treeMapCount(const TreeMap* map);
int treeMapIteratorSize(const TreeMap* map);
void treeMapInsert(TreeMap* map, int key, void* value);
void* nullable treeMapSearchKey(const TreeMap* map, int key);
void* nullable treeMapSearchMinOrMax(const TreeMap* map, bool minOrMax, int* nullable key);
void treeMapDelete(TreeMap* map, int key);
void treeMapIterateBegin(TreeMap* map, TreeMapIterator* iterator);
#define treeMapIterateBegin(x, y) treeMapIterateBegin(x, (y = xalloca2(treeMapIteratorSize(x))))
void* nullable treeMapIterate(TreeMapIterator* iterator); // assumed readonly - don't do any write operations while iterating, iteration involves mutex read-locking
void treeMapIterateEnd(TreeMapIterator* iterator);
void treeMapDestroy(TreeMap* map);
