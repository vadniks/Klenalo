
#include "rwMutex.h"
#include "hashtable.h"

// inspired by the Java standard library's Hashtable

typedef struct Node {
    const int hash;
    void* value;
    struct Node* nullable next;
} Node;

struct _Hashtable {
    Node* nullable* nodes;
    int capacity, count;
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

int hashtableHash(const byte* value, int size) {
    int hash = 0;
    for (; size--; hash = 31 * hash + *value++);
    return hash;
}

Hashtable* hashtableCreate(const bool synchronized, const HashtableDeallocator nullable deallocator) {
    Hashtable* const hashtable = xmalloc(sizeof *hashtable);
    assert(hashtable);
    assert(hashtable->nodes = xcalloc((hashtable->capacity = INITIAL_CAPACITY), sizeof(void*)));
    hashtable->count = 0;
    unconst(hashtable->deallocator) = deallocator;
    unconst(hashtable->rwMutex) = synchronized ? rwMutexCreate() : nullptr;
    hashtable->iterating = false;
    return hashtable;
}

static inline void deallocateValue(const Hashtable* const hashtable, void* const value) {
    if (hashtable->deallocator) hashtable->deallocator(value);
}

static inline void xRwMutexCommand(Hashtable* const hashtable, const RWMutexCommand command) {
    if (hashtable->rwMutex) rwMutexCommand(hashtable->rwMutex, command);
}

static int makeIndex(const int capacity, const int hash) {
    assert(capacity);
    return (hash & SINT32_MAX) % capacity; // cannot be negative or exceed the capacity
}

static void rehash(Hashtable* const hashtable) {
    if (hashtable->capacity == SINT32_MAX) return;

    const int newCapacity = hashtable->capacity * 2 + 1;
    if (newCapacity < hashtable->capacity) return; // overflow

    Node** const newNodes = xcalloc(newCapacity, sizeof(void*));
    assert(newNodes);

    for (int index = 0; index < hashtable->capacity; index++) {
        for (Node* node = hashtable->nodes[index]; node;) {
            Node* const nextNode = node->next;

            const int newIndex = makeIndex(newCapacity, node->hash);
            node->next = newNodes[newIndex];
            newNodes[newIndex] = node;

            node = nextNode;
        }
    }

    hashtable->capacity = newCapacity;
    xfree(hashtable->nodes);
    hashtable->nodes = newNodes;
}

void hashtablePut(Hashtable* const hashtable, const int hash, void* const value) {
    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(
        hashtable->capacity >= INITIAL_CAPACITY &&
        hashtable->capacity < SINT32_MAX &&
        hashtable->count < SINT32_MAX &&
        hashtable->nodes &&
        !hashtable->iterating
    );

    Node** const anchor = (void*) hashtable->nodes + makeIndex(hashtable->capacity, hash);

    for (Node* node = *anchor; node; node = node->next) {
        if (node->hash != hash) continue;

        deallocateValue(hashtable, node->value);
        node->value = value;

        xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return;
    }

    Node* const node = xmalloc(sizeof *node);
    assert(node);
    assignToStructWithConsts(node, hash, value, *anchor)
    *anchor = node;
    hashtable->count++;

    if (hashtable->count >= (int) ((float) hashtable->capacity * LOAD_FACTOR)) rehash(hashtable);

    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void* nullable hashtableGet(Hashtable* const hashtable, const int hash) {
    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_LOCK);
    assert(hashtable->capacity && hashtable->count && hashtable->nodes);

    for (const Node* node = hashtable->nodes[makeIndex(hashtable->capacity, hash)]; node; node = node->next) {
        if (node->hash != hash) continue;
        xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_UNLOCK);
        return node->value;
    }

    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_UNLOCK);
    return nullptr;
}

void hashtableRemove(Hashtable* const hashtable, const int hash) {
    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(hashtable->capacity && hashtable->count && hashtable->nodes && !hashtable->iterating);

    Node** const anchor = (void*) hashtable->nodes + makeIndex(hashtable->capacity, hash);

    for (Node* node = *anchor, * previous = nullptr; node; previous = node, node = node->next) {
        if (node->hash != hash) continue;

        if (previous) previous->next = node->next;
        else *anchor = node->next;

        deallocateValue(hashtable, node->value);

        xfree(node);
        hashtable->count--;
        break;
    }

    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

int hashtableCapacity(Hashtable* const hashtable) {
    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_LOCK);
    const int capacity = hashtable->capacity;
    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_UNLOCK);
    return capacity;
}

int hashtableCount(Hashtable* const hashtable) {
    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_LOCK);
    const int count = hashtable->count;
    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_UNLOCK);
    return count;
}

HashtableIterator* hashtableIteratorCreate(Hashtable* const hashtable) {
    xRwMutexCommand(hashtable, RW_MUTEX_COMMAND_READ_LOCK);
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
        if ((iterator->node = iterator->hashtable->nodes[iterator->index++])) {
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

    xRwMutexCommand(iterator->hashtable, RW_MUTEX_COMMAND_READ_UNLOCK);
}

void hashtableDestroy(Hashtable* const hashtable) {
    if (hashtable->rwMutex) rwMutexDestroy(hashtable->rwMutex);
    assert(!hashtable->iterating);

    for (int index = 0; index < hashtable->capacity; index++) {
        for (Node* node = hashtable->nodes[index]; node; node = node->next) {
            deallocateValue(hashtable, node->value);
            xfree(node);
        }
    }

    xfree(hashtable->nodes);
    xfree(hashtable);
}

// TODO: add shrinkToFit() - allocate only minimal nodes to fit all the values so that each node contains only one value and call it after remove()
// TODO: add find() or contains()
