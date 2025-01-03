
#pragma once

#include "defs.h"

typedef void (* HashtableDeallocator)(void* const);

struct _Hashtable;
typedef struct _Hashtable Hashtable;

int hashtableHash(const byte* key, int size);
Hashtable* hashtableInit(const HashtableDeallocator nullable deallocator);
void hashtablePut(Hashtable* const hashtable, const int hash, void* const value);
void* nullable hashtableGet(const Hashtable* const hashtable, const int hash);
void hashtableRemove(Hashtable* const hashtable, const int hash);
int hashtableSize(const Hashtable* const hashtable);
void hashtableDestroy(Hashtable* const hashtable);
