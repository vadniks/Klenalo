
// TODO: remove internal mutex from all collections
#include "hashtable.h"

typedef struct Node {
    const unsigned long key;
    void* value;
    int height;
    struct Node
        * nullable left,
        * nullable right;
} Node;

struct _Hashtable {
    const Allocator* const internalAllocator;
    const Deallocator nullable deallocator;
    Node* nullable* buckets;
    int capacity, count; // capacity - power of two actual allocated items, count - inserted items from outside
};

struct _HashtableIterator {
    Hashtable* const hashtable;
    int index;
    Node* nullable node;
};

const int HASHTABLE_ITERATOR_SIZE = sizeof(HashtableIterator);
static const int INITIAL_CAPACITY = 8;

Hashtable* hashtableCreate(const Allocator* const internalAllocator, const Deallocator nullable valueDeallocator) {
    Hashtable* const hashtable = xmalloc(sizeof *hashtable);
    unconst(hashtable->internalAllocator) = internalAllocator;
    unconst(hashtable->deallocator) = valueDeallocator;
    hashtable->buckets = xmalloc((hashtable->capacity = INITIAL_CAPACITY) * sizeof(void*));
    hashtable->count = 0;
    return hashtable;
}

static unsigned long calcHash(const unsigned long value) {
    // Fowler–Noll–Vo (FNV‑1a) Hash Function

    const unsigned long
        FNV_OFFSET_BASIS = 0xcbf29ce484222325ULL,
        FNV_PRIME = 0x100000001b3ULL,
        SEED = 932579;

    unsigned long hash = SEED ^ FNV_OFFSET_BASIS;

    for (byte i = 0; i < (byte) sizeof(long); i++) {
        hash ^= ((const byte*) &value)[i];
        hash *= FNV_PRIME;
    }

    return hash;
}

static inline int calcIndex(const unsigned long hash, const int capacity) {
    return (int) (hash & (capacity - 1));
}

void hashtablePut(Hashtable* const hashtable, const unsigned long key, void* const value) {
    const unsigned long hash = calcHash(key);
    const int index = calcIndex(hash, hashtable->capacity);

//    Node* const root = insert(hashtable->buckets[index], key, );
}

void* nullable hashtableGet(Hashtable* const hashtable, const unsigned long hash) {
    return nullptr;
}

void* nullable hashtableRemove(Hashtable* const hashtable, const unsigned long hash, const bool deallocate) {
    return nullptr;
}

int hashtableCapacity(Hashtable* const hashtable) {
    return 0;
}

int hashtableCount(Hashtable* const hashtable) {
    return 0;
}

#undef hashtableIterateBegin
void hashtableIterateBegin(Hashtable* const hashtable, HashtableIterator* const iterator) {

}

void* nullable hashtableIterate(HashtableIterator* const iterator) {
    return nullptr;
}

void hashtableIterateEnd(HashtableIterator* const iterator) {

}

void hashtableDestroy(Hashtable* const hashtable) {

}
