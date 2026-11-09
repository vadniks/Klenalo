
#include "deque.h"

typedef struct Node {
    struct Node* nullable prev, * nullable next;
    void* value;
} Node;

struct _Deque {
    const Deallocator nullable deallocator;
    Node* nullable head, * nullable tail;
    int count;
};

static constexpr int MAX_SIZE = ~0u / 2u; // 0x7fffffff

Deque* dequeCreate(const Deallocator nullable deallocator) {
    Deque* const deque = malloc(sizeof *deque);
    unconst(deque->deallocator) = deallocator;
    deque->head = nullptr;
    deque->tail = nullptr;
    return deque;
}

static void deallocateValue(const Deque* const deque, void* const value) {
    if (deque->deallocator) deque->deallocator(value);
}

static Node* nodeCreate(void* nullable const value) {
    Node* const node = malloc(sizeof *node);
    node->prev = node->next = nullptr;
    node->value = value;
    return node;
}

static Node* nullable nodePrevOrNull(const Node* nullable const node) {
    if (!node || !node->prev) return nullptr;
    return node->prev;
}

static void nodeTrySetPrev(Node* nullable const target, Node* nullable prev) {
    if (target && target->prev) target->prev = prev;
}

static Node* nullable nodeNextOrNull(const Node* nullable const node) {
    if (!node || !node->next) return nullptr;
    return node->next;
}

static void nodeTrySetNext(Node* nullable const target, Node* nullable next) {
    if (target && target->next) target->next = next;
}

void dequePushBack(Deque* const deque, void* const value) {
    assert(deque->count < MAX_SIZE);
    Node* const new = nodeCreate(value);

    Node* const prev = deque->tail;
    nodeTrySetNext(prev, new);
    new->prev = prev;

    deque->tail = new;
    deque->count++;
}

void dequePushFront(Deque* const deque, void* const value) {
    assert(deque->count < MAX_SIZE);
    Node* const new = nodeCreate(value);

    Node* const next = deque->head;
    nodeTrySetPrev(next, new);
    new->next = next;

    deque->head = new;
    deque->count++;
}

int dequeSize(const Deque* const deque) {
    return deque->count;
}

void* nullable dequePopFirst(Deque* const deque, const bool delete) {
    Node* const first = deque->head;
    if (!first) return nullptr;

    Node* const next = first->next;
    deque->head = next;
    nodeTrySetPrev(next, nullptr);

    deque->count--;

    void* const value = first->value;
    free(first);

    if (delete) {
        deallocateValue(deque, value);
        return nullptr;
    }
    return value;
}

void* nullable dequePopLast(Deque* const deque, const bool delete) {
    Node* const last = deque->tail;
    if (!last) return nullptr;

    Node* const prev = last->prev;
    deque->tail = prev;
    nodeTrySetNext(prev, nullptr);

    deque->count--;

    void* const value = last->value;
    free(last);

    if (delete) {
        deallocateValue(deque, value);
        return nullptr;
    }
    return value;
}

void* nullable dequePeekFirst(const Deque* const deque) {
    const Node* const node = deque->head;
    return node ? node->value : nullptr;
}

void* nullable dequePeekLast(const Deque* const deque) {
    const Node* const node = deque->tail;
    return node ? node->value : nullptr;
}

void dequeClear(Deque* const deque) {
    if (!deque->count) return;

    Node* node = deque->head;
    while (node) {
        deallocateValue(deque, node->value);
        free(node);
        node = node->next;
    }

    deque->count = 0;
}

void dequeDestroy(Deque* const deque) {
    dequeClear(deque);
    free(deque);
}
