
#include "rwMutex.h"
#include "hashtable.h"

// inspired by Java's Hashtable

typedef struct Node {
    const int hash;
    void* value;
    struct Node* nullable next;
} Node;

struct _Hashtable {
    Node* nullable* table;
    int capacity, count, threshold;
    const HashtableDeallocator nullable deallocator;
    RWMutex* const nullable rwMutex;
    bool iterating;
};

struct _HashtableIterator {
    Hashtable* const hashtable;
    int index;
    Node* nullable node;
};

static const int SINT32_MAX = ~0u / 2u; // 0x7fffffff
static const int INITIAL_CAPACITY = 11;
static const float LOAD_FACTOR = 0.75f;

int hashtableHash(const byte* key, int size) {
    int hash = 0;
    for (; size--; hash = 31 * hash + *key++);
    return hash;
}

Hashtable* hashtableCreate(const bool synchronized, const HashtableDeallocator nullable deallocator) {
    Hashtable* const hashtable = xmalloc(sizeof *hashtable);
    assert(hashtable);
    assert(hashtable->table = xcalloc((hashtable->capacity = INITIAL_CAPACITY), sizeof(void*)));
    hashtable->count = 0;
    hashtable->threshold = (int) ((float) INITIAL_CAPACITY * LOAD_FACTOR);
    *(HashtableDeallocator*) &hashtable->deallocator = deallocator;
    *(RWMutex**) &hashtable->rwMutex = synchronized ? rwMutexCreate() : nullptr;
    hashtable->iterating = false;
    return hashtable;
}

static inline void deallocateValue(const Hashtable* const hashtable, void* const value) {
    if (hashtable->deallocator) hashtable->deallocator(value);
}

static inline void hashtableRwMutexCommand(Hashtable* const hashtable, const RWMutexCommand command) {
    if (hashtable->rwMutex) rwMutexCommand(hashtable->rwMutex, command);
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

    Node** newTable = xcalloc(newCapacity, sizeof(void*));
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
    xfree(hashtable->table);
    hashtable->table = newTable;
    hashtable->capacity = newCapacity;
}

void hashtablePut(Hashtable* const hashtable, const int hash, void* const value) {
    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_WRITE_LOCK);
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

        deallocateValue(hashtable, node->value);
        node->value = value;

        hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return;
    }

    Node* const new = xmalloc(sizeof *new);
    assert(new);
    xmemcpy(new, &(Node) {hash, value, *anchor}, sizeof(Node));
    *anchor = new;
    hashtable->count++;

    if (hashtable->count >= hashtable->threshold)
        rehash(hashtable);

    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void* nullable hashtableGet(Hashtable* const hashtable, const int hash) {
    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_LOCK);
    assert(hashtable->capacity && hashtable->count && hashtable->table);

    for (const Node* node = hashtable->table[makeIndex(hashtable->capacity, hash)]; node; node = node->next) {
        if (node->hash == hash) {
            hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_UNLOCK);
            return node->value;
        }
    }

    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_UNLOCK);
    return nullptr;
}

void hashtableRemove(Hashtable* const hashtable, const int hash) {
    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(hashtable->capacity && hashtable->count && hashtable->table && !hashtable->iterating);

    Node** const anchor = &hashtable->table[makeIndex(hashtable->capacity, hash)];

    for (Node* node = *anchor, * previous = nullptr; node; previous = node, node = node->next) {
        if (node->hash != hash) continue;

        if (previous) previous->next = node->next;
        else *anchor = node->next;

        deallocateValue(hashtable, node->value);

        xfree(node);
        hashtable->count--;
        break;
    }

    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

int hashtableCapacity(Hashtable* const hashtable) {
    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_LOCK);
    const int capacity = hashtable->capacity;
    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_UNLOCK);
    return capacity;
}

int hashtableCount(Hashtable* const hashtable) {
    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_LOCK);
    const int count = hashtable->count;
    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_UNLOCK);
    return count;
}

HashtableIterator* hashtableIteratorCreate(Hashtable* const hashtable) {
    hashtableRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_LOCK);
    assert(hashtable->count && !hashtable->iterating);

    hashtable->iterating = true;

    HashtableIterator* const iterator = xmalloc(sizeof *iterator);
    assert(iterator);
    *(Hashtable**) &iterator->hashtable = hashtable;
    iterator->index = 0;
    iterator->node = nullptr;
    return iterator;
}

void* nullable hashtableIterate(HashtableIterator* const iterator) {
    assert(iterator->hashtable->iterating);
    if (iterator->index >= iterator->hashtable->capacity) return nullptr;

    Node* node;

    if (iterator->node) {
        node = iterator->node;
        iterator->node = node->next;
        return node->value;
    }

    while (iterator->index < iterator->hashtable->capacity) {
        if ((iterator->node = iterator->hashtable->table[iterator->index++])) {
            node = iterator->node;
            iterator->node = node->next;
            return node->value;
        }
    }

    return nullptr;
}

void hashtableIteratorDestroy(HashtableIterator* const iterator) {
    assert(iterator->hashtable->iterating);
    iterator->hashtable->iterating = false;
    xfree(iterator);

    hashtableRwMutexCommand(iterator->hashtable, RW_MUTEX_COMMAND_READ_UNLOCK);
}

void hashtableDestroy(Hashtable* const hashtable) {
    if (hashtable->rwMutex) rwMutexDestroy(hashtable->rwMutex);
    assert(!hashtable->iterating);

    for (int index = 0; index < hashtable->capacity; index++) {
        for (Node* node = hashtable->table[index]; node; node = node->next) {
            deallocateValue(hashtable, node->value);
            xfree(node);
        }
    }

    xfree(hashtable->table);
    xfree(hashtable);
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
            for (Node* node = hashtable->table[i]; node; node = node->next, j++);
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

    {
        Hashtable* const hashtable = hashtableCreate(nullptr);
        assert(hashtable);

        hashtable->table[0] = SDL_malloc(sizeof(Node));
        hashtable->table[0]->value = (void*) 1;
        hashtable->table[0]->next = SDL_malloc(sizeof(Node));
        hashtable->table[0]->next->value = (void*) 2;
        hashtable->table[0]->next->next = nullptr;

        hashtable->table[1] = SDL_malloc(sizeof(Node));
        hashtable->table[1]->value = (void*) 3;
        hashtable->table[1]->next = nullptr;

        hashtable->table[2] = SDL_malloc(sizeof(Node));
        hashtable->table[2]->value = (void*) 4;
        hashtable->table[2]->next = SDL_malloc(sizeof(Node));
        hashtable->table[2]->next->value = (void*) 5;
        hashtable->table[2]->next->next = SDL_malloc(sizeof(Node));
        hashtable->table[2]->next->next->value = (void*) 6;
        hashtable->table[2]->next->next->next = nullptr;

        hashtable->count = 6;

        HashtableIterator* const iterator = hashtableIteratorCreate(hashtable);
        assert(iterator);

        int i = 0;
        int* value;
        while ((value = hashtableIterate(iterator))) {
            assert((int) value == i + 1);
            i++;
        }

        assert(i == hashtableCount(hashtable));
        hashtableIteratorDestroy(iterator);

        hashtableDestroy(hashtable);
    }

    assert(allocations == SDL_GetNumAllocations());
}
#endif
