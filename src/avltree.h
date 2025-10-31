
#pragma once

#include "defs.h"

// AVL Tree (self-balancing binary search tree), optionally thread-safe, only works with non-null values

typedef int (* AvltreeComparator)(const void* const, const void* const);
typedef void* (* AvltreeValueDuplicator)(const void* const);

struct _Avltree;
typedef struct _Avltree Avltree;

struct _AvltreeTraverser;
typedef struct _AvltreeTraverser AvltreeTraverser;

extern const int AVLTREE_TRAVERSER_SIZE;

Avltree* avltreeCreate(const bool synchronized);
void avltreeInsert(Avltree* const tree, const void* const value);
overloadable void* nullable avltreeSearch(Avltree* const tree, const void* const key, const AvltreeComparator comparator);
overloadable void* nullable avltreeSearch(Avltree* const tree, const bool minOrMax, const AvltreeComparator comparator);
void avltreeDelete(Avltree* const tree, const void* const value);
void avltreeTraverseBegin(Avltree* const tree, AvltreeTraverser* const traverser);
#define avltreeTraverseBegin(x, y) avltreeTraverseBegin(x, (y = xalloca2(AVLTREE_TRAVERSER_SIZE)))
void* nullable avlTreeTraverse(AvltreeTraverser* const traverser);
void avltreeTraverseEnd(AvltreeTraverser* const traverser);
void avltreeDestroy(Avltree* const tree);
