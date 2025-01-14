
#include <SDL2/SDL_stdinc.h>
#include "hashtable.h"

typedef struct Node {
    int hash; // const
    void* value;
    struct Node* nullable next;
} Node;

struct _Hashtable {
    Node* nullable* table;
    int capacity, count, threshold;
    HashtableDeallocator nullable deallocator;
    bool iterating;
};

struct _HashtableIterator {
    Hashtable* hashtable;
    int index;
    Node* nullable node;
};

static const int SINT32_MAX = 0x7fffffff;
static const int INITIAL_CAPACITY = 11;
static const float LOAD_FACTOR = 0.75f;

int hashtableHash(const byte* key, int size) {
    int hash = 0;
    for (; size--; hash = 31 * hash + *key++);
    return hash;
}

Hashtable* hashtableCreate(const HashtableDeallocator nullable deallocator) {
    Hashtable* const hashtable = SDL_malloc(sizeof *hashtable);
    assert(hashtable);
    assert(hashtable->table = SDL_calloc((hashtable->capacity = INITIAL_CAPACITY), sizeof(void*)));
    hashtable->count = 0;
    hashtable->threshold = (int) ((float) INITIAL_CAPACITY * LOAD_FACTOR);
    hashtable->deallocator = deallocator;
    hashtable->iterating = false;
    return hashtable;
}

static int makeIndex(const int capacity, const int hash) {
    assert(capacity);
    return (hash & SINT32_MAX) % capacity; // cannot be negative or exceed the capacity
}

static void rehash(Hashtable* const hashtable) {
    const int oldCapacity = hashtable->capacity;
    if (oldCapacity == SINT32_MAX) return;

    const int newCapacity = oldCapacity * 2 + 1;
    if (newCapacity < oldCapacity) return;

    Node** newTable = SDL_calloc(newCapacity, sizeof(void*));
    assert(newTable);

    for (int index = 0; index < oldCapacity; index++) {
        for (Node* anchor = hashtable->table[index]; anchor;) {
            Node* const current = anchor;
            anchor = anchor->next;

            const int newIndex = makeIndex(newCapacity, current->hash);
            current->next = newTable[newIndex];
            newTable[newIndex] = current;
        }
    }

    hashtable->threshold = (int) ((float) newCapacity * LOAD_FACTOR);
    SDL_free(hashtable->table);
    hashtable->table = newTable;
    hashtable->capacity = newCapacity;
}

void hashtablePut(Hashtable* const hashtable, const int hash, void* const value) {
    assert(
        hashtable->capacity >= INITIAL_CAPACITY &&
        hashtable->capacity < SINT32_MAX &&
        hashtable->count < SINT32_MAX &&
        hashtable->table &&
        !hashtable->iterating
    );

    Node** const anchor = &hashtable->table[makeIndex(hashtable->capacity, hash)];

    for (Node* node = *anchor; node; node = node->next) {
        if (node->hash != hash) continue;

        if (hashtable->deallocator)
            hashtable->deallocator(node->value);

        node->value = value;
        return;
    }

    Node* const new = SDL_malloc(sizeof *new);
    assert(new);
    *new = (Node) {hash, value, *anchor};
    *anchor = new;
    hashtable->count++;

    if (hashtable->count >= hashtable->threshold)
        rehash(hashtable);
}

void* nullable hashtableGet(const Hashtable* const hashtable, const int hash) {
    assert(hashtable->capacity && hashtable->count && hashtable->table);

    for (const Node* node = hashtable->table[makeIndex(hashtable->capacity, hash)]; node; node = node->next) {
        if (node->hash == hash)
            return node->value;
    }

    return nullptr;
}

void hashtableRemove(Hashtable* const hashtable, const int hash) {
    assert(hashtable->capacity && hashtable->count && hashtable->table && !hashtable->iterating);

    Node** const anchor = &hashtable->table[makeIndex(hashtable->capacity, hash)];

    for (Node* node = *anchor, * previous = nullptr; node; previous = node, node = node->next) {
        if (node->hash != hash) continue;

        if (previous) previous->next = node->next;
        else *anchor = node->next;

        if (hashtable->deallocator)
            hashtable->deallocator(node->value);

        SDL_free(node);
        hashtable->count--;
        break;
    }
}

int hashtableCapacity(const Hashtable* const hashtable) {
    return hashtable->capacity;
}

int hashtableCount(const Hashtable* const hashtable) {
    return hashtable->count;
}

HashtableIterator* hashtableIteratorCreate(Hashtable* const hashtable) {
    assert(hashtable->count && !hashtable->iterating);

    hashtable->iterating = true;

    HashtableIterator* const iterator = SDL_malloc(sizeof *iterator);
    assert(iterator);
    iterator->hashtable = hashtable;
    iterator->index = 0;
    iterator->node = nullptr;
    return iterator;
}

void* nullable hashtableIterate(HashtableIterator* const iterator) {
    assert(iterator->hashtable->iterating);
    if (iterator->index >= iterator->hashtable->capacity) return nullptr;

    if (iterator->node) {
        Node* const node = iterator->node;
        iterator->node = node->next;
        return node->value;
    }

    while (iterator->index < iterator->hashtable->capacity) {
        if ((iterator->node = iterator->hashtable->table[iterator->index++])) {
            Node* const node = iterator->node;
            iterator->node = node->next;
            return node->value;
        }
    }

    return nullptr;
}

void hashtableIteratorDestroy(HashtableIterator* const iterator) {
    assert(iterator->hashtable->iterating);
    iterator->hashtable->iterating = false;
    SDL_free(iterator);
}

void hashtableDestroy(Hashtable* const hashtable) {
    assert(!hashtable->iterating);

    for (int index = 0; index < hashtable->capacity; index++) {
        for (Node* node = hashtable->table[index]; node; node = node->next) {
            if (hashtable->deallocator)
                hashtable->deallocator(node->value);
            SDL_free(node);
        }
    }

    SDL_free(hashtable->table);
    SDL_free(hashtable);
}


// TODO: add shrinkToFit() - allocate only minimal nodes to fit all the values so that each node contains only one value and call it after remove()
// TODO: add find() or contains()


#if TESTING
static void deallocator(void* const value) {
    SDL_free(value);
}

void hashtableRunTests(void) {
    const int allocations = SDL_GetNumAllocations();

    {
        Hashtable* const hashtable = hashtableCreate(deallocator);
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
//        printf("%d\n", hashtableCapacity(hashtable));

        {
            HashtableIterator* const iterator = hashtableIteratorCreate(hashtable);
            int i = 0;
            int* value;
            while ((value = hashtableIterate(iterator))) {
                assert(value == hashtableGet(hashtable, hashtableHash((byte*) value, sizeof(int))));
                i++;
            }
            assert(i == hashtableCount(hashtable));
            hashtableIteratorDestroy(iterator);
        }

        for (int i = 0; i < hashtableCapacity(hashtable); i++) {
            int j = 0;
            for (Node* node = hashtable->table[i]; node; node = node->next)
                j++;
            assert(j <= 2);
        }

        hashtableRemove(hashtable, hashtableHash((byte*) (int[1]) {0}, sizeof(int)));
        hashtableRemove(hashtable, hashtableHash((byte*) (int[1]) {5}, sizeof(int)));
        hashtableRemove(hashtable, hashtableHash((byte*) (int[1]) {11}, sizeof(int)));
        hashtableRemove(hashtable, hashtableHash((byte*) (int[1]) {99}, sizeof(int)));

        assert(hashtableCount(hashtable) == 96);

        hashtablePut(hashtable, hashtableHash((byte*) (int[1]) {1}, sizeof(int)), SDL_malloc(sizeof(int)));
        hashtablePut(hashtable, hashtableHash((byte*) (int[1]) {50}, sizeof(int)), SDL_malloc(sizeof(int)));
        hashtablePut(hashtable, hashtableHash((byte*) (int[1]) {90}, sizeof(int)), SDL_malloc(sizeof(int)));

        hashtableDestroy(hashtable);
    }

    {
        Hashtable* const hashtable = hashtableCreate(deallocator);
        assert(hashtable);

        for (int i = 0; i < 100; i++) {
            int* const j = SDL_malloc(sizeof(int));
            *j = i;
//            printf("%d\n", hashtableCapacity(hashtable));
            hashtablePut(hashtable, hashtableHash((byte*) j, sizeof(int)), j);
        }

        for (int i = 0; i < 00; i++)
            hashtableRemove(hashtable, hashtableHash((byte*) &i, sizeof(int)));

        hashtableDestroy(hashtable);
    }

    assert(allocations == SDL_GetNumAllocations());
}
#endif
