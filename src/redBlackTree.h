
#pragma once

#include "defs.h"

struct _RedBlackTree;
typedef struct _RedBlackTree RedBlackTree;

typedef void (* RedBlackTreeDeallocator)(void* const);

RedBlackTree* redBlackTreeInit(const RedBlackTreeDeallocator nullable deallocator);
void redBlackTreeInsert(RedBlackTree* const redBlackTree, const int key, void* const value);
void* nullable redBlackTreeGet(const RedBlackTree* const redBlackTree, const int key);
void redBlackTreeDestroy(RedBlackTree* const redBlackTree);
