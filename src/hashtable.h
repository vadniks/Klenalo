
#pragma once

#include "defs.h"

// optionally thread-safe, only works with non-null values

typedef void (* HashtableDeallocator)(void* const);

struct _Hashtable;
typedef struct _Hashtable Hashtable;

struct _HashtableIterator;
typedef struct _HashtableIterator HashtableIterator;

extern const int HASHTABLE_ITERATOR_SIZE;

int hashtableHash(const byte* value, int size);
#define hashtableHashPrimitive(x) hashtableHash((const byte*) (typeof(x)[1]) {x}, _Generic((x), byte: 1, short: 2, int: 4, long: 8))

Hashtable* hashtableCreate(const bool synchronized, const HashtableDeallocator nullable deallocator);
void hashtablePut(Hashtable* const hashtable, const int hash, void* const value); // hashes are the keys and they must be unique
void* nullable hashtableGet(Hashtable* const hashtable, const int hash);
void hashtableRemove(Hashtable* const hashtable, const int hash);
int hashtableCapacity(Hashtable* const hashtable); // amount of currently allocated cells for storing collision buckets
int hashtableCount(Hashtable* const hashtable); // amount of elements being stored
void hashtableIterateBegin(Hashtable* const hashtable, HashtableIterator* const iterator); // don't use put or remove while iterator is active, fails if there's another active iterator, fails if there's no items
#define hashtableIterateBegin(x) hashtableIterateBegin(x, xalloca(HASHTABLE_ITERATOR_SIZE))
void* nullable hashtableIterate(HashtableIterator* const iterator); // returns null when there aren't any more items available
void hashtableIterateEnd(HashtableIterator* const iterator); // must be called while its hashtable is still active
void hashtableDestroy(Hashtable* const hashtable);
