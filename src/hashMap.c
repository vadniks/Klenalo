
#include <SDL2/SDL_stdinc.h>
#include "hashMap.h"

static const int MAX_SIZE = 0x7fffffff;

typedef struct {
    void* value;
    int key;
} Item;

typedef struct {
    Item* items; // TODO: binary search tree instead of linear list or sort on insertion
    int size;
} Bucket;

struct _HashMap {
    Bucket* buckets;
    int size;
    int total;
    HashMapDeallocator nullable deallocator;
};

int hashMapHash(const byte* value, int size) {
    int hash = 1;
    for (; size--; hash = 31 * hash + *value++); // from Java
    return hash;
}

HashMap* hashMapInit(const HashMapDeallocator nullable deallocator) {
    HashMap* const hashMap = SDL_malloc(sizeof *hashMap);
    assert(hashMap);
    hashMap->buckets = nullptr;
    hashMap->size = 0;
    hashMap->total = 0;
    hashMap->deallocator = deallocator;
    return hashMap;
}

static int hash(const int key) {
    return key ^ (int) ((unsigned) key >> 16u); // from Java
}

static int makeIndex(const HashMap* const hashMap, const int key) {
    assert(hashMap->size);
    return hash(key) % hashMap->size;
}

static int compareItems(const void* const a, const void* const b) {
    const int aa = ((const Item*) a)->key;
    const int bb = ((const Item*) b)->key;
    return (aa > bb) - (aa < bb);
}

void hashMapPut(HashMap* const hashMap, const int key, void* const value) {
    assert(hashMap->size < MAX_SIZE && hashMap->total < MAX_SIZE);

    const int index = makeIndex(hashMap, key);
    const Item item = {value, key};

    if (index >= hashMap->size) {
        assert(hashMap->buckets = SDL_realloc(hashMap->buckets, (index + 1) * sizeof(Bucket))); // TODO: rehash

        for (; hashMap->size < index - 1; hashMap->size++) {
            Bucket* const bucket = &hashMap->buckets[hashMap->size];
            bucket->items = nullptr;
            bucket->size = 0;
        }
        hashMap->size += 2;

        Bucket* const bucket = &hashMap->buckets[index];
        assert(bucket->items = SDL_malloc(sizeof(Item)));
        bucket->items[0] = item;
        bucket->size = 1;
    } else {
        Bucket* const bucket = &hashMap->buckets[index];
        assert(bucket->size < MAX_SIZE);
        assert(bucket->items = SDL_realloc(bucket->items, ++bucket->size * sizeof(Item))); // TODO: rehash
        bucket->items[bucket->size - 1] = item;

        SDL_qsort(bucket->items, bucket->size, sizeof(Item), compareItems);
    }

    hashMap->total++;
}

void* nullable hashMapGet(const HashMap* const hashMap, const int key) {
    if (!hashMap->size) return nullptr;

    const int index = makeIndex(hashMap, key);
    if (index < 0 || index >= hashMap->size) return nullptr;

    const Bucket* const bucket = &hashMap->buckets[index];
    if (bucket->size == 1) return bucket->items[0].value;

    const Item* const item =
        SDL_bsearch(&(Item) {.key = key}, bucket->items, bucket->size, sizeof(Item), compareItems);
    return item ? item->value : nullptr;
}

void hashMapRemove(HashMap* const hashMap, const int key) {

}

int hashMapSize(const HashMap* const hashMap) {

}

void hashMapDestroy(HashMap* const hashMap) {

}
