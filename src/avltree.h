
#pragma once

#include "defs.h"

// AVL Tree (self-balancing binary search tree), optionally thread-safe, only works with non-null values

typedef int (* AvltreeComparator)(const void* const, const void* const); // a=first < b=second : negative, a = b : zero, a > b : positive
typedef void (* AvltreeVisitor)(const void* const);
typedef void (* AvltreeDeallocator)(void* const);

typedef struct _AvltreeNode AvltreeNode;
typedef struct _Avltree Avltree;
typedef struct _AvltreeTraverser AvltreeTraverser;

extern const int AVLTREE_TRAVERSER_SIZE;

Avltree* avltreeCreate(const AvltreeComparator comparator, const bool synchronized, const AvltreeDeallocator nullable deallocator);
int avltreeElements(Avltree* const tree);
AvltreeNode* avltreeInsert(Avltree* const tree, AvltreeNode* nullable node, const void* const value);
void* nullable avltreeSearchKey(Avltree* const tree, const void* const key);
void* nullable avltreeSearchMinOrMax(Avltree* const tree, const bool minOrMax);
void avltreeDelete(Avltree* const tree, const void* const value);
void avltreeTraverse(Avltree* const tree, const AvltreeNode* nullable const node, const AvltreeVisitor visitor);
void avltreeTraverseBegin(Avltree* const tree, AvltreeTraverser* const traverser);
#define avltreeTraverseBegin(x, y) avltreeTraverseBegin(x, (y = xalloca2(AVLTREE_TRAVERSER_SIZE)))
void* nullable avltreeTraverseLinearly(AvltreeTraverser* const traverser);
void avltreeTraverseEnd(AvltreeTraverser* const traverser);
void avltreeDestroy(Avltree* const tree, AvltreeNode* nullable const node);
