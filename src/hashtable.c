
#include <SDL2/SDL_stdinc.h>
#include "hashtable.h"

typedef struct {
    void* value;
    int hash;
} Item;

typedef struct {
    Item* items; // TODO: binary search tree (or red black tree) instead of linear list or sort on insertion
    int size;
} Bucket;

struct _Hashtable {
    Bucket* buckets;
    int size;
    int total;
    HashtableDeallocator nullable deallocator;
};

static const int MAX_SIZE = 0x7fffffff;

int hashtableHash(const byte* key, int size) {
    int hash = 1;
    for (; size--; hash = 31 * hash + *key++); // from Java
    return hash;
}

Hashtable* hashtableInit(const HashtableDeallocator nullable deallocator) {
    Hashtable* const hashtable = SDL_malloc(sizeof *hashtable);
    assert(hashtable);
    hashtable->buckets = nullptr;
    hashtable->size = 0;
    hashtable->total = 0;
    hashtable->deallocator = deallocator;
    return hashtable;
}

static int makeIndex(const Hashtable* const hashtable, const int hash) {
    assert(hashtable->size);
    return (hash & MAX_SIZE) % hashtable->size;
}

static int compareItems(const void* const a, const void* const b) {
    const int aa = ((const Item*) a)->hash;
    const int bb = ((const Item*) b)->hash;
    return (aa > bb) - (aa < bb);
}

void hashtablePut(Hashtable* const hashtable, const int hash, void* const value) {
    assert(hashtable->size < MAX_SIZE && hashtable->total < MAX_SIZE);

    // TODO *

    const int index = makeIndex(hashtable, hash);
    const Item item = {value, hash};

    if (index >= hashtable->size) {
        assert(hashtable->buckets = SDL_realloc(hashtable->buckets, (index + 1) * sizeof(Bucket))); // TODO: rehash

        for (; hashtable->size < index - 1; hashtable->size++) {
            Bucket* const bucket = &hashtable->buckets[hashtable->size];
            bucket->items = nullptr;
            bucket->size = 0;
        }
        hashtable->size += 2;

        Bucket* const bucket = &hashtable->buckets[index];
        assert(bucket->items = SDL_malloc(sizeof(Item)));
        bucket->items[0] = item;
        bucket->size = 1;
    } else {
        Bucket* const bucket = &hashtable->buckets[index];
        assert(bucket->size < MAX_SIZE);
        assert(bucket->items = SDL_realloc(bucket->items, ++bucket->size * sizeof(Item))); // TODO: rehash
        bucket->items[bucket->size - 1] = item;

        SDL_qsort(bucket->items, bucket->size, sizeof(Item), compareItems);
    }

    hashtable->total++;
}

void* nullable hashtableGet(const Hashtable* const hashtable, const int hash) {
    if (!hashtable->size) return nullptr;

    const int index = makeIndex(hashtable, hash);
    if (index < 0 || index >= hashtable->size) return nullptr;

    const Bucket* const bucket = &hashtable->buckets[index];
    if (bucket->size == 1) return bucket->items[0].value;

    const Item* const item =
        SDL_bsearch(&(Item) {.hash = hash}, bucket->items, bucket->size, sizeof(Item), compareItems);
    return item ? item->value : nullptr;
}

void hashtableRemove(Hashtable* const hashtable, const int hash) {

}

int hashtableSize(const Hashtable* const hashtable) {

}

void hashtableDestroy(Hashtable* const hashtable) {

}
