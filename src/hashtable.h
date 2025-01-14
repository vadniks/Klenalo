
#pragma once

#include "defs.h"

// not thread-safe, not intended to store nulls

typedef void (* HashtableDeallocator)(void* const);

struct _Hashtable;
typedef struct _Hashtable Hashtable;

struct _HashtableIterator;
typedef struct _HashtableIterator HashtableIterator;

int hashtableHash(const byte* key, int size);
Hashtable* hashtableCreate(const HashtableDeallocator nullable deallocator);
void hashtablePut(Hashtable* const hashtable, const int hash, void* const value); // hashes are the keys and they must be unique
void* nullable hashtableGet(const Hashtable* const hashtable, const int hash);
void hashtableRemove(Hashtable* const hashtable, const int hash);
int hashtableCapacity(const Hashtable* const hashtable); // amount of currently allocated cells for storing collision buckets
int hashtableCount(const Hashtable* const hashtable); // amount of elements being stored
HashtableIterator* hashtableIteratorCreate(Hashtable* const hashtable); // don't use put or remove while iterator is active, fails if there's another active iterator, fails if there's no items
void* nullable hashtableIterate(HashtableIterator* const iterator); // returns null when there aren't any more items available
void hashtableIteratorDestroy(HashtableIterator* const iterator); // must be called while its hashtable is still active
void hashtableDestroy(Hashtable* const hashtable);

#if TESTING
void hashtableRunTests(void);
#endif
