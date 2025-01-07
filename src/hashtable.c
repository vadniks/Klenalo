
#include <SDL2/SDL_stdinc.h>
#include "hashtable.h"

// TODO: binary search (or red black) tree instead of linear (or linked) list or sort on insertion and use binary search

// Based on Java's Hashtable

typedef struct _Entry {
    int hash;
    void* value;
    struct _Entry* nullable next;
} Entry;

struct _Hashtable {
    Entry* nullable* table;
    int capacity;
    int count;
    int threshold;
    HashtableDeallocator nullable deallocator;
};

static const int SINT32_MAX = 0x7fffffff;
static const int INITIAL_CAPACITY = 11;
static const float LOAD_FACTOR = 0.75f;

int hashtableHash(const byte* key, int size) {
    int hash = 1;
    for (; size--; hash = 31 * hash + *key++);
    return hash;
}

Hashtable* hashtableInit(const HashtableDeallocator nullable deallocator) {
    Hashtable* const hashtable = SDL_malloc(sizeof *hashtable);
    assert(hashtable);
    hashtable->table = SDL_calloc((hashtable->capacity = INITIAL_CAPACITY), sizeof(void*));
    hashtable->count = 0;
    hashtable->threshold = (int) ((float) INITIAL_CAPACITY * LOAD_FACTOR);
    hashtable->deallocator = deallocator;
    return hashtable;
}

static int makeIndex(const Hashtable* const hashtable, const int hash) {
    assert(hashtable->capacity);
    return (hash & SINT32_MAX) % hashtable->capacity; // cannot be negative
}

static void rehash(Hashtable* const hashtable) {
    const int oldCapacity = hashtable->capacity;
    if (oldCapacity == SINT32_MAX) return;

    const int newCapacity = (oldCapacity << 1) + 1;
    if (newCapacity <= oldCapacity || newCapacity == SINT32_MAX) return;

    Entry** const oldTable = hashtable->table;

    Entry** const newTable = SDL_calloc(newCapacity, sizeof(void*));
    assert(newCapacity);

    hashtable->threshold = (int) ((float) newCapacity * LOAD_FACTOR);
    hashtable->table = newTable;
    hashtable->capacity = newCapacity;

    for (int i = oldCapacity; i-- > 0;) {
        for (Entry* old = oldTable[i]; old;) {
            Entry* entry = old;
            old = old->next;

            const int index = makeIndex(hashtable, entry->hash);
            entry->next = newTable[index];
            newTable[index] = entry;
        }
    }

    SDL_free(oldTable);
}

void hashtablePut(Hashtable* const hashtable, const int hash, void* const value) {
    assert(
        hashtable->capacity >= INITIAL_CAPACITY &&
        hashtable->capacity < SINT32_MAX &&
        hashtable->count < SINT32_MAX &&
        hashtable->table
    );

    int index = makeIndex(hashtable, hash);
    for (Entry* entry = hashtable->table[index]; entry; entry = entry->next) {
        if (entry->hash == hash) {
            if (hashtable->deallocator)
                hashtable->deallocator(entry->value);
            entry->value = value;
            return;
        }
    }

    if (hashtable->count >= hashtable->threshold) {
        rehash(hashtable);
        index = makeIndex(hashtable, hash);
    }

    Entry* const previous = hashtable->table[index];
    hashtable->table[index] = SDL_malloc(sizeof(void*));
    *hashtable->table[index] = (Entry) {hash, value, previous};
    hashtable->count++;
}

void* nullable hashtableGet(const Hashtable* const hashtable, const int hash) {
    assert(hashtable->capacity && hashtable->count && hashtable->table);
    const int index = makeIndex(hashtable, hash);

    for (Entry* entry = hashtable->table[index]; entry; entry = entry->next) {
        if (entry->hash == hash)
            return entry->value;
    }

    return nullptr;
}

void hashtableRemove(Hashtable* const hashtable, const int hash) {
    assert(hashtable->capacity && hashtable->count && hashtable->table);
    const int index = makeIndex(hashtable, hash);

    for (Entry* entry = hashtable->table[index], * previous = nullptr; entry; previous = entry, entry = entry->next) {
        if (entry->hash != hash) continue;

        if (previous) previous->next = entry->next;
        else hashtable->table[index] = entry->next;

        hashtable->count--;
        if (hashtable->deallocator)
            hashtable->deallocator(entry->value);
        SDL_free(entry);

        break;
    }
}

int hashtableCapacity(const Hashtable* const hashtable) {
    return hashtable->capacity;
}

int hashtableCount(const Hashtable* const hashtable) {
    return hashtable->count;
}

void hashtableDestroy(Hashtable* const hashtable) {
    for (int index = 0; index < hashtable->capacity; index++) {
        for (Entry* entry = hashtable->table[index]; entry; entry = entry->next) {
            if (hashtable->deallocator)
                hashtable->deallocator(entry->value);
            SDL_free(entry);
        }
    }

    SDL_free(hashtable->table);
    SDL_free(hashtable);
}


// TODO: add shrinkToFit() - allocate only minimal entries to fit all the values so that each entry contains only one value and call it after remove()
// TODO: add find() or contains()


#if TESTING
static void deallocator(void* const value) {
    SDL_free(value);
}

void hashtableRunTests(void) {
    const int allocations = SDL_GetNumAllocations();

    Hashtable* const hashtable = hashtableInit(deallocator);
    assert(hashtable);

    for (int i = 0; i < 100; i++) {
        int* const j = SDL_malloc(sizeof(int));
        *j = i;
        hashtablePut(hashtable, hashtableHash((byte*) j, sizeof(int)), j);
    }

    for (int i = 0; i < 100; i++) {
        const int j = hashtableHash((byte*) &i, sizeof(int));
        const int* const k = hashtableGet(hashtable, j);
        assert(k && *k == i);
    }

    assert(hashtableCount(hashtable) == 100);
//    printf("%d\n", hashtableCapacity(hashtable));

    for (int i = 0; i < hashtableCapacity(hashtable); i++) {
        int j = 0;
        for (Entry* entry = hashtable->table[i]; entry; entry = entry->next)
            j++;
        assert(j <= 2);
    }

    hashtableRemove(hashtable, hashtableHash((byte*) (int[1]) {0}, sizeof(int)));
    hashtableRemove(hashtable, hashtableHash((byte*) (int[1]) {5}, sizeof(int)));
    hashtableRemove(hashtable, hashtableHash((byte*) (int[1]) {11}, sizeof(int)));
    hashtableRemove(hashtable, hashtableHash((byte*) (int[1]) {99}, sizeof(int)));

    assert(hashtableCount(hashtable) == 96);

    hashtableDestroy(hashtable);

    assert(allocations == SDL_GetNumAllocations());
}
#endif
