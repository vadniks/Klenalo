
#pragma once

#include "defs.h"

// Red-Black Tree (self-balancing binary search tree), optionally thread-safe, only works with non-null and unique values
// TODO: embed in the Hashtable, replacing the linked list

typedef void (* TreeMapVisitor)(const int key, const void* const value);
typedef void (* TreeMapDeallocator)(void* const);

typedef struct _TreeMap TreeMap;

TreeMap* treeMapCreate(const TreeMapDeallocator nullable deallocator);
void treeMapInsert(TreeMap* const map, const int key, void* const value);
void* nullable treeMapSearchKey(TreeMap* const map, const int key);
void* nullable treeMapSearchMinOrMax(TreeMap* const map, const bool minOrMax);
void treeMapDelete(TreeMap* const map, const int key);
void treeMapTraverse(TreeMap* const map, const TreeMapVisitor visitor);
void treeMapDestroy(TreeMap* const map);
