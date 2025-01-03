
#include <SDL2/SDL_stdinc.h>
#include "hashtable.h"

// TODO: binary search (or red black) tree instead of linear (or linked) list or sort on insertion

// Based on Java's Hashtable

typedef struct _Entry {
    int hash;
    void* value;
    struct _Entry* nullable next;
} Entry;

struct _Hashtable {
    Entry** table;
    int tableSize;
    int valueCount;
    int threshold;
    HashtableDeallocator nullable deallocator;
};

static const int SINT32_MAX = 0x7fffffff;
static const int INITIAL_SIZE = 11;
static const float LOAD_FACTOR = 0.75f;

int hashtableHash(const byte* key, int size) {
    int hash = 1;
    for (; size--; hash = 31 * hash + *key++);
    return hash;
}

Hashtable* hashtableInit(const HashtableDeallocator nullable deallocator) {
    Hashtable* const hashtable = SDL_malloc(sizeof *hashtable);
    assert(hashtable);
    hashtable->table = SDL_calloc((hashtable->tableSize = INITIAL_SIZE), sizeof(void*));
    hashtable->valueCount = 0;
    hashtable->threshold = (int) ((float) INITIAL_SIZE * LOAD_FACTOR);
    hashtable->deallocator = deallocator;
    return hashtable;
}

static int makeIndex(const Hashtable* const hashtable, const int hash) {
    assert(hashtable->tableSize);
    return (hash & SINT32_MAX) % hashtable->tableSize;
}

static void rehash(Hashtable* const hashtable) {
    const int oldTableSize = hashtable->tableSize;
    if (oldTableSize == SINT32_MAX) return;

    Entry** const oldTable = hashtable->table;

    int newTableSize = (oldTableSize << 1) + 1;
    if (newTableSize <= oldTableSize) return;

    Entry** const newTable = SDL_calloc(newTableSize, sizeof(void*));
    assert(newTableSize);

    hashtable->threshold = (int) ((float) newTableSize * LOAD_FACTOR);
    hashtable->table = newTable;
    hashtable->tableSize = newTableSize;

    for (int i = oldTableSize; i-- > 0;) {
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
    assert(hashtable->tableSize < SINT32_MAX && hashtable->valueCount < SINT32_MAX);

    int index = makeIndex(hashtable, hash);
    for (Entry* entry = hashtable->table[index]; entry; entry = entry->next) {
        if (entry->hash == hash) {
            entry->value = value;
            return;
        }
    }

    if (hashtable->valueCount >= hashtable->threshold) {
        rehash(hashtable);
        index = makeIndex(hashtable, hash);
    }

    Entry* previous = hashtable->table[index];
    hashtable->table[index] = SDL_malloc(sizeof(void*));
    *hashtable->table[index] = (Entry) {hash, value, previous};
    hashtable->valueCount++;
}

void* nullable hashtableGet(const Hashtable* const hashtable, const int hash) {

}

void hashtableRemove(Hashtable* const hashtable, const int hash) {

}

int hashtableSize(const Hashtable* const hashtable) {

}

void hashtableDestroy(Hashtable* const hashtable) {

}
