
#pragma once

#include "defs.h"

// Red-Black Tree (self-balancing binary search tree), optionally thread-safe, only works with non-null and unique values
// TODO: embed in the Hashtable, replacing the linked list

typedef int (* TreeMapComparator)(const void* const a, const void* const b); // a=first < b=second : negative, a = b : zero, a > b : positive
typedef void (* TreeMapVisitor)(const void* const);
typedef void (* TreeMapDeallocator)(void* const);

typedef struct _TreeMapNode TreeMapNode;

TreeMapNode* treeMapInsert(TreeMapNode* nullable head, void* const value, const TreeMapComparator nullable comparator, const TreeMapDeallocator nullable deallocator);
void* nullable treeMapSearchKey(TreeMapNode* const head, const void* const key);
void* nullable treeMapSearchMinOrMax(TreeMapNode* const head, const bool minOrMax);
void treeMapDelete(TreeMapNode* const head, const void* const value);
void treeMapTraverse(const TreeMapNode* const head, const TreeMapVisitor visitor);
void treeMapDestroy(TreeMapNode* const head);
