
#include "../rwMutex.h"
#include "treeMap.h"

// inspired by the Java standard library's TreeMap

typedef enum : byte {
    COLOR_RED,
    COLOR_BLACK
} Color;

typedef struct _Node {
    const int key;
    void* const value;
    struct _Node
        * nullable left, // or could have used NIL instead - nonullable stub node to avoid numerous null checks
        * nullable parent,
        * nullable right;
    Color color;
} Node;

struct _TreeMap {
    const Allocator* const internalAllocator;
    const Deallocator nullable deallocator;
    RWMutex* nullable const rwMutex;
    Node* nullable root;
    int count;
    bool iterating;
};

struct _TreeMapIterator {
    TreeMap* const map;
    int currentStackTop;
    Node* nullable stack[];
};

static const int MAX_SIZE = ~0u / 2u; // 0x7fffffff

TreeMap* treeMapCreate(const Allocator* const internalAllocator, const bool synchronized, const Deallocator nullable deallocator) {
    TreeMap* const map = internalAllocator->malloc(sizeof *map);
    unconst(map->internalAllocator) = internalAllocator;
    unconst(map->deallocator) = deallocator;
    unconst(map->rwMutex) = synchronized ? rwMutexCreate() : nullptr;
    map->root = nullptr;
    map->count = 0;
    map->iterating = false;
    return map;
}

static inline void xRwMutexCommand(TreeMap* const map, const RWMutexCommand command) {
    if (map->rwMutex) rwMutexCommand(map->rwMutex, command);
}

int treeMapCount(TreeMap* const map) {
    xRwMutexCommand(map, RW_MUTEX_COMMAND_READ_LOCK);
    const int count = map->count;
    xRwMutexCommand(map, RW_MUTEX_COMMAND_READ_UNLOCK);
    return count;
}

static int iteratorSize(TreeMap* const map) {
    return (int) sizeof(TreeMapIterator) + (int) sizeof(Node*) * map->count;
}

int treeMapIteratorSize(TreeMap* const map) {
    xRwMutexCommand(map, RW_MUTEX_COMMAND_READ_LOCK);
    const int size = iteratorSize(map);
    xRwMutexCommand(map, RW_MUTEX_COMMAND_READ_UNLOCK);
    return size;
}

static Node* nodeCreate(TreeMap* const map, const int key, void* const value) {
    Node* const node = map->internalAllocator->malloc(sizeof *node);

    unconst(node->key) = key;
    unconst(node->value) = value;
    node->parent = node->left = node->right = nullptr;
    node->color = COLOR_RED;

    map->count++;

    return node;
}

static void nodeDestroy(TreeMap* const map, Node* nullable const node) {
    if (!node) return;

    assert(map->count >= 0);
    map->count--;

    if (map->deallocator) map->deallocator(node->value);
    map->internalAllocator->free(node);
}

static inline Node* nullable parentOf(const Node* nullable const node) {
    return node ? node->parent : nullptr;
}

static inline Node* nullable leftOf(const Node* nullable const node) {
    return node ? node->left : nullptr;
}

static inline Node* nullable rightOf(const Node* nullable const node) {
    return node ? node->right : nullptr;
}

static inline void setParentOf(Node* nullable const node, Node* nullable const other) {
    if (node) node->parent = other;
}

static inline void setLeftOf(Node* nullable const node, Node* nullable const other) {
    if (node) node->left = other;
}

static inline void setRightOf(Node* nullable const node, Node* nullable const other) {
    if (node) node->right = other;
}

static inline Color colorOf(const Node* nullable const node) {
    return node ? node->color : COLOR_BLACK;
}

static inline void setColorOf(Node* nullable const node, const Color color) {
    if (node) node->color = color;
}

static void rotateLeft(TreeMap* const map, Node* nullable const x) {
    if (!x) return;

    Node* const y = x->right;
    x->right = leftOf(y);
    if (leftOf(y)) y->left->parent = x;

    setParentOf(y, x->parent);
    if (!x->parent) map->root = y;
    else if (x == leftOf(parentOf(x))) setLeftOf(x->parent, y);
    else setRightOf(x->parent, y);

    setLeftOf(y, x);
    x->parent = y;
}

static void rotateRight(TreeMap* const map, Node* nullable const y) {
    if (!y) return;

    Node* const x = y->left;
    y->left = rightOf(x);
    if (rightOf(x)) x->right->parent = y;

    setParentOf(x, y->parent);
    if (!y->parent) map->root = x;
    else if (y == rightOf(parentOf(y))) setRightOf(y->parent, x);
    else setLeftOf(y->parent, x);

    setRightOf(x, y);
    y->parent = x;
}

static void insertFixup(TreeMap* const map, Node* nullable z) {
    if (!z) return;
    while (colorOf(parentOf(z)) == COLOR_RED) {
        if (parentOf(z) == leftOf(parentOf(parentOf(z)))) {
            Node* const y = rightOf(parentOf(parentOf(z)));
            if (colorOf(y) == COLOR_RED) {
                setColorOf(parentOf(z), COLOR_BLACK);
                setColorOf(y, COLOR_BLACK);
                setColorOf(parentOf(parentOf(z)), COLOR_RED);
                z = parentOf(parentOf(z));
            } else {
                if (z == rightOf(parentOf(z))) {
                    z = parentOf(z);
                    rotateLeft(map, z);
                }
                setColorOf(parentOf(z), COLOR_BLACK);
                setColorOf(parentOf(parentOf(z)), COLOR_RED);
                rotateRight(map, parentOf(parentOf(z)));
            }
        } else {
            Node* const y = leftOf(parentOf(parentOf(z)));
            if (colorOf(y) == COLOR_RED) {
                setColorOf(parentOf(z), COLOR_BLACK);
                setColorOf(y, COLOR_BLACK);
                setColorOf(parentOf(parentOf(z)), COLOR_RED);
                z = parentOf(parentOf(z));
            } else {
                if (z == leftOf(parentOf(z))) {
                    z = parentOf(z);
                    rotateRight(map, z);
                }
                setColorOf(parentOf(z), COLOR_BLACK);
                setColorOf(parentOf(parentOf(z)), COLOR_RED);
                rotateLeft(map, parentOf(parentOf(z)));
            }
        }
    }
    map->root->color = COLOR_BLACK;
}

void treeMapInsert(TreeMap* const map, const int key, void* const value) {
    xRwMutexCommand(map, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(map->count < MAX_SIZE && !map->iterating);

    Node* x = map->root, * y = nullptr, * new = nodeCreate(map, key, value);

    while (x) {
        y = x;
        if (key < x->key) x = x->left;
        else if (key > x->key) x = x->right;
        else assert(false); /*{
            xRwMutexCommand(map, RW_MUTEX_COMMAND_WRITE_UNLOCK);
            return;
        }*/
    }

    if (!y) map->root = new;
    else {
        if (key < y->key) y->left = new;
        else if (key > y->key) y->right = new;
        else assert(false);
        new->parent = y;
    }

    insertFixup(map, new);
    xRwMutexCommand(map, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

static Node* nullable searchKey(const TreeMap* const map, const int key) {
    Node* node = map->root;
    while (node) {
        if (key < node->key) node = node->left;
        else if (key > node->key) node = node->right;
        else return node;
    }
    return nullptr;
}

void* nullable treeMapSearchKey(TreeMap* const map, const int key) {
    xRwMutexCommand(map, RW_MUTEX_COMMAND_READ_LOCK);
    Node* const node = searchKey(map, key);
    xRwMutexCommand(map, RW_MUTEX_COMMAND_READ_UNLOCK);
    return node ? node->value : nullptr;
}

static Node* nullable searchMinOrMax(Node* nullable node, const bool minOrMax) {
    if (!node) return nullptr;
    while (minOrMax ? node->left : node->right)
        minOrMax ? (node = node->left) : (node = node->right);
    return node;
}

void* nullable treeMapSearchMinOrMax(TreeMap* const map, const bool minOrMax, int* nullable const key) {
    xRwMutexCommand(map, RW_MUTEX_COMMAND_READ_LOCK);
    Node* const node = searchMinOrMax(map->root, minOrMax);
    xRwMutexCommand(map, RW_MUTEX_COMMAND_READ_UNLOCK);

    if (node) {
        if (key) *key = node->key;
        return node->value;
    } else {
        if (key) *key = 0;
        return nullptr;
    }
}

static void transplant(TreeMap* const map, Node* nullable const u, Node* nullable const v) {
    if (!parentOf(u)) map->root = v;
    else if (u == leftOf(parentOf(u))) setLeftOf(parentOf(u), v);
    else setRightOf(parentOf(u), v);
    setParentOf(v, parentOf(u));
}

static void deleteFixup(TreeMap* const map, Node* nullable x) {
    if (!x) return;
    while (x != map->root && colorOf(x) == COLOR_BLACK) {
        if (x == leftOf(parentOf(x))) {
            Node* w = rightOf(parentOf(x));
            if (colorOf(w) == COLOR_RED) {
                setColorOf(w, COLOR_BLACK);
                setColorOf(parentOf(x), COLOR_RED);
                rotateLeft(map, parentOf(x));
                w = rightOf(parentOf(x));
            }
            if (colorOf(leftOf(w)) == COLOR_BLACK && colorOf(rightOf(w)) == COLOR_BLACK) {
                setColorOf(w, COLOR_RED);
                x = parentOf(x);
            } else {
                if (colorOf(rightOf(w)) == COLOR_BLACK) {
                    setColorOf(leftOf(w), COLOR_BLACK);
                    setColorOf(w, COLOR_RED);
                    rotateRight(map, w);
                    w = rightOf(parentOf(x));
                }
                setColorOf(w, colorOf(parentOf(x)));
                setColorOf(parentOf(x), COLOR_BLACK);
                setColorOf(rightOf(w), COLOR_BLACK);
                rotateLeft(map, parentOf(x));
                x = map->root;
            }
        } else {
            Node* w = leftOf(parentOf(x));
            if (colorOf(w) == COLOR_RED) {
                setColorOf(w, COLOR_BLACK);
                setColorOf(parentOf(x), COLOR_RED);
                rotateRight(map, parentOf(x));
                w = leftOf(parentOf(x));
            }
            if (colorOf(rightOf(w)) == COLOR_BLACK && colorOf(leftOf(w)) == COLOR_BLACK) {
                setColorOf(w, COLOR_RED);
                x = parentOf(x);
            } else {
                if (colorOf(leftOf(w)) == COLOR_BLACK) {
                    setColorOf(rightOf(w), COLOR_BLACK);
                    setColorOf(w, COLOR_RED);
                    rotateLeft(map, w);
                    w = leftOf(parentOf(x));
                }
                setColorOf(w, colorOf(parentOf(x)));
                setColorOf(parentOf(x), COLOR_BLACK);
                setColorOf(leftOf(w), COLOR_BLACK);
                rotateRight(map, parentOf(x));
                x = map->root;
            }
        }
    }
    setColorOf(x, COLOR_BLACK);
}

void treeMapDelete(TreeMap* const map, const int key) {
    xRwMutexCommand(map, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(!map->iterating);

    Node* const found = searchKey(map, key);
    if (!found) {
        xRwMutexCommand(map, RW_MUTEX_COMMAND_WRITE_UNLOCK);
        return;
    }

    Node* y = found, * x;
    Color color = y->color;

    if (!found->left) {
        x = found->right;
        transplant(map, found, found->right);
    } else if (!found->right) {
        x = found->left;
        transplant(map, found, found->left);
    } else {
        y = searchMinOrMax(found->right, true);
        color = y->color;
        x = y->right;

        if (y->parent == found)
            setParentOf(x, y);
        else {
            transplant(map, y, y->right);
            y->right = found->right;
            setParentOf(y->right, y);
        }

        transplant(map, found, y);
        y->left = found->left;
        setParentOf(y->left, y);
        y->color = found->color;
    }

    if (color == COLOR_BLACK)
        deleteFixup(map, x);

    nodeDestroy(map, found);
    xRwMutexCommand(map, RW_MUTEX_COMMAND_WRITE_UNLOCK);
}

static void iteratePutLeftChildrenIntoStack(TreeMapIterator* const iterator, Node* nullable node) {
    while (node) {
        assert(iterator->currentStackTop + 1 <= iterator->map->count);
        iterator->stack[++iterator->currentStackTop] = node;
        node = node->left;
    }
}

static inline void clearIteratorStack(TreeMapIterator* const iterator) {
    xmemset(iterator->stack, 0, sizeof(Node*) * iterator->map->count);
}

#undef treeMapIterateBegin
void treeMapIterateBegin(TreeMap* const map, TreeMapIterator* const iterator) {
    xRwMutexCommand(map, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(!map->iterating);
    map->iterating = true;
    xRwMutexCommand(map, RW_MUTEX_COMMAND_WRITE_UNLOCK);

    xRwMutexCommand(map, RW_MUTEX_COMMAND_READ_LOCK);

    unconst(iterator->map) = map;
    iterator->currentStackTop = -1;
    clearIteratorStack(iterator);

    iteratePutLeftChildrenIntoStack(iterator, map->root);
}

void* nullable treeMapIterate(TreeMapIterator* const iterator) {
    assert(iterator->map->iterating);
    if (iterator->currentStackTop < 0) return nullptr;

    Node* const node = iterator->stack[iterator->currentStackTop--];

    if (rightOf(node))
        iteratePutLeftChildrenIntoStack(iterator, node->right);

    return node ? node->value : nullptr;
}

void treeMapIterateEnd(TreeMapIterator* const iterator) {
    xRwMutexCommand(iterator->map, RW_MUTEX_COMMAND_READ_UNLOCK);

    xRwMutexCommand(iterator->map, RW_MUTEX_COMMAND_WRITE_LOCK);
    assert(iterator->map->iterating);
    iterator->map->iterating = false;
    xRwMutexCommand(iterator->map, RW_MUTEX_COMMAND_WRITE_UNLOCK);

    iterator->currentStackTop = -1;
    clearIteratorStack(iterator);
}

void treeMapDestroy(TreeMap* const map) {
    if (map->rwMutex) {
        rwMutexDestroy(map->rwMutex);
        unconst(map->rwMutex) = nullptr; // so the following dealing with iterator won't mess up the mutex
    }

    assert(!map->iterating);

    TreeMapIterator* const iterator = xalloca2(iteratorSize(map));
    treeMapIterateBegin(map, iterator);

    Node* node;
    while ((node = treeMapIterate(iterator)))
        nodeDestroy(map, node);

    treeMapIterateEnd(iterator); // unnecessary

    map->internalAllocator->free(map);
}
