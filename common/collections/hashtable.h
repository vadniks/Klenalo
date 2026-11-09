
#pragma once

#include "../defs.h"

// Key-to-value mapping of items through hashing, only works with non-null values

typedef struct _Hashtable Hashtable;
typedef struct _HashtableIterator HashtableIterator;

extern const int HASHTABLE_ITERATOR_SIZE;

Hashtable* hashtableCreate(Deallocator nullable deallocator);
void hashtablePut(Hashtable* hashtable, unsigned long key, void* value); // hashes are the keys and they must be unique
void* nullable hashtableGet(Hashtable* hashtable, unsigned long hash);
void* nullable hashtableRemove(Hashtable* hashtable, unsigned long hash, bool deallocate);
int hashtableCapacity(Hashtable* hashtable); // amount of currently allocated cells for storing collision buckets
int hashtableCount(Hashtable* hashtable); // amount of elements being stored
void hashtableIterateBegin(Hashtable* hashtable, HashtableIterator* iterator); // don't use put or remove while iterator is active, fails if there's another active iterator, fails if there's no items
#define hashtableIterateBegin(x, y) hashtableIterateBegin(x, (y = xalloca2(HASHTABLE_ITERATOR_SIZE)))
void* nullable hashtableIterate(HashtableIterator* iterator); // returns null when there aren't any more items available
void hashtableIterateEnd(HashtableIterator* iterator); // must be called while its hashtable is still active
void hashtableDestroy(Hashtable* hashtable);
