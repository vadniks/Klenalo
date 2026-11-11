
#pragma once

#include "../defs.h"

// Red-Black Tree (self-balancing binary search tree), only works with non-null and unique values

typedef struct _Treemap Treemap;
typedef struct _TreemapIterator TreemapIterator;

Treemap* treemapCreate(Deallocator nullable deallocator);
int treemapCount(const Treemap* map);
int treemapIteratorSize(const Treemap* map);
void treemapInsert(Treemap* map, int key, void* value);
void* nullable treemapSearchKey(const Treemap* map, int key);
void* nullable treemapSearchMinOrMax(const Treemap* map, bool minOrMax, int* nullable key);
void treemapDelete(Treemap* map, int key);
void treemapIterateBegin(Treemap* map, TreemapIterator* iterator);
#define treemapIterateBegin(x, y) treemapIterateBegin(x, (y = xalloca2(treemapIteratorSize(x))))
void* nullable treemapIterate(TreemapIterator* iterator); // assumed readonly - don't do any write operations while iterating, iteration involves mutex read-locking
void treemapIterateEnd(TreemapIterator* iterator);
void treemapDestroy(Treemap* map);
