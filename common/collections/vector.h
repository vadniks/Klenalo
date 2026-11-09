
#pragma once

#include "../defs.h"

// Linear vector (dynamically resizable array - aka vector), queue and stack operations support [deprecated], only works with non-null values

typedef struct _Vector Vector;

Vector* vectorCreate(Deallocator nullable deallocator);
Vector* nullable vectorCopy(const Vector* old, Duplicator nullable duplicator);
void vectorAddBack(Vector* vector, void* value); // vectorAdd, stackPush
void vectorAddFront(Vector* vector, void* value); // queuePush
void* nullable vectorGet(const Vector* vector, int index);
void vectorSwap(const Vector* vector, int index1, int index2); // swap places items with corresponding indexes - first goes to second's position and v.v.
void* nullable vectorPopFirst(Vector* vector); // queuePop // TODO: rename to remove(First|Last)
void* nullable vectorPopLast(Vector* vector); // stackPop
void vectorRemove(Vector* vector, int index); // don't use in a loop - ConcurrentModificationException - or each iteration adjust loop counter
void* nullable vectorPeekFirst(const Vector* vector); // queuePeek
void* nullable vectorPeekLast(const Vector* vector); // stackPeek
int vectorSize(const Vector* vector);
void* nullable vectorBinarySearch(const Vector* vector, const void* key, Comparator comparator);
void vectorQSort(Vector* vector, Comparator comparator);
void vectorClear(Vector* vector);
void vectorDestroy(Vector* vector);
