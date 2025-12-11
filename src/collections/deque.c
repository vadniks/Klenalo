
#include "../rwMutex.h"
#include "deque.h"

typedef struct _Node {
    void* const value;
    struct _Node
        * nullable previous,
        * nullable next;
} Node;

struct _Deque {
    Node
        * nullable first,
        * nullable last;
    int size;
    RWMutex* nullable const rwMutex;
    const Deallocator nullable deallocator;
};

static const int MAX_SIZE = ~0u / 2u; // 0x7fffffff

Deque* dequeCreate(const bool synchronized, const Deallocator deallocator) {
    Deque* const deque = xmalloc(sizeof *deque);
    deque->first = deque->last = nullptr;
    deque->size = 0;
    unconst(deque->rwMutex) = synchronized ? rwMutexCreate() : nullptr;
    unconst(deque->deallocator) = deallocator;
    return deque;
}

static inline void deallocateValue(const Deque* const deque, void* const value) {
    if (deque->deallocator) deque->deallocator(value);
}

static inline void xRwMutexCommand(Deque* const deque, const RWMutexCommand command) {
    if (deque->rwMutex) rwMutexCommand(deque->rwMutex, command);
}

void dequePushBack(Deque* const deque, void* const value) {
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(deque->size < MAX_SIZE);

    Node* const node = xmalloc(sizeof *node);
    unconst(node->value) = value;
    node->next = nullptr;
    node->previous = deque->last;

    if (deque->last)
        deque->last->next = node;
    deque->last = node;

    if (!deque->first)
        deque->first = node;

    deque->size++;
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void dequePushFront(Deque* const deque, void* const value) {
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(deque->size < MAX_SIZE);

    Node* const node = xmalloc(sizeof *node);
    unconst(node->value) = value;
    node->next = deque->first;
    node->previous = nullptr;

    if (deque->first)
        deque->first->previous = node;
    deque->first = node;

    if (!deque->last)
        deque->last = node;

    deque->size++;
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

static Node* nullable search(Deque* const deque, const int index, const bool fromStartOrFromEnd) {
    int i = fromStartOrFromEnd ? 0 : deque->size - 1;
    for (
        Node* node = fromStartOrFromEnd ? deque->first : deque->last;
        node;
        node = fromStartOrFromEnd ? node->next : node->previous, fromStartOrFromEnd ? i++ : i--
    ) {
        if (i == index)
            return node;
    }
    return nullptr;
}

void* nullable dequeGet(Deque* const deque, const int index, const bool fromStartOrFromEnd) {
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_READ_LOCK);
    Node* const node = search(deque, index, fromStartOrFromEnd);
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_READ_UNLOCK);
    return node ? node->value : nullptr;
}

int dequeSize(Deque* const deque) {
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_READ_LOCK);
    const int size = deque->size;
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_READ_UNLOCK);
    return size;
}

void* nullable dequePopFirst(Deque* const deque) {
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_LOCK);
    if (!deque->size) {
        xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return nullptr;
    } else
        assert(deque->first);

    void* const value = deque->first->value;
    xfree(deque->first);
    deque->first = deque->first->next;
    deque->first->previous = nullptr;

    deque->size--;

    if (deque->size == 1) {
        deque->last = deque->first;
        deque->last->next = nullptr;
    } else if (!deque->size) {
        deque->last = nullptr;
        deque->first->next = nullptr;
    }

    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_UNLOCK);
    return value;
}

void* nullable dequePopLast(Deque* const deque) {
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_LOCK);
    if (!deque->size) {
        xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return nullptr;
    } else
        assert(deque->last);

    void* const value = deque->last->value;
    xfree(deque->last);
    deque->last = deque->last->previous;
    deque->last->next = nullptr;

    deque->size--;

    if (deque->size == 1) {
        deque->first = deque->last;
        deque->first->previous = nullptr;
    } else if (!deque->size) {
        deque->first = nullptr;
        deque->last->previous = nullptr;
    }

    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_UNLOCK);
    return value;
}

void dequeRemove(Deque* const deque, const int index) {
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_LOCK);

    Node* const node = search(deque, index, deque->size - index > index);
    if (!node) {
        xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return;
    }

    Node* const previous = node->previous;
    Node* const next = node->next;

    if (previous) previous->next = next;
    if (next) next->previous = previous;

    if (deque->first == node)
        deque->first = next;
    if (deque->last == node)
        deque->last = previous;

    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_UNLOCK);

    deallocateValue(deque, node->value);
    xfree(node);
}

void* nullable dequePeekFirst(Deque* const deque) {
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_READ_LOCK);
    Node* const node = deque->first;
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_READ_UNLOCK);
    return node ? node->value : nullptr;
}

void* nullable dequePeekLast(Deque* const deque) {
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_READ_LOCK);
    Node* const node = deque->last;
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_READ_UNLOCK);
    return node ? node->value : nullptr;
}

static void destroyNodes(Deque* const deque) {
    for (
        Node* node = deque->first;
        node;
        node = node->next
    ) {
        deallocateValue(deque, node->value);
        xfree(node);
    }
}

void dequeClear(Deque* const deque) {
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_LOCK);
    destroyNodes(deque);
    deque->size = 0;
    xRwMutexCommand(deque, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

void dequeDestroy(Deque* const deque) {
    if (deque->rwMutex) rwMutexDestroy(deque->rwMutex);
    destroyNodes(deque);
    xfree(deque);
}
