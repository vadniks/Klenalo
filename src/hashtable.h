
#pragma once

#include "defs.h"

// not thread-safe, null values are not supported

typedef void (* HashtableDeallocator)(void* const);

struct _Hashtable;
typedef struct _Hashtable Hashtable;

int hashtableHash(const byte* key, int size); // aka Java Object's default hashCode()
Hashtable* hashtableInit(const HashtableDeallocator deallocator);
void hashtablePut(Hashtable* const hashtable, const int hash, void* const value); // hashes are the keys and they must be unique
void* nullable hashtableGet(const Hashtable* const hashtable, const int hash);
void hashtableRemove(Hashtable* const hashtable, const int hash);
int hashtableCapacity(const Hashtable* const hashtable); // amount of currently allocated cells for storing collision buckets
int hashtableCount(const Hashtable* const hashtable); // amount of elements being stored
void hashtableDestroy(Hashtable* const hashtable);

#if TESTING
void hashtableRunTests(void);
#endif
