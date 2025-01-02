
#pragma once

#include "defs.h"

struct _HashMap;
typedef struct _HashMap HashMap;

typedef void (* HashMapDeallocator)(void* const);

int hashMapHash(const byte* value, int size);
HashMap* hashMapInit(const HashMapDeallocator nullable deallocator);
void hashMapPut(HashMap* const hashMap, const int key, void* const value);
void* nullable hashMapGet(const HashMap* const hashMap, const int key);
void hashMapRemove(HashMap* const hashMap, const int key);
int hashMapSize(const HashMap* const hashMap);
void hashMapDestroy(HashMap* const hashMap);
