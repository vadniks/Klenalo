
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
        * nullable left,
        * nullable parent,
        * nullable right;
    Color color;
} Node;

struct _TreeMap {
    const TreeMapDeallocator nullable deallocator;
    Node* nullable root;
    int count;
};

struct _TreeMapIterator {
    TreeMap* const map;
    int currentStackTop;
    Node* nullable stack[];
};

TreeMap* treeMapCreate(const TreeMapDeallocator nullable deallocator) {
    TreeMap* const map = xmalloc(sizeof *map);
    assert(map);
    unconst(map->deallocator) = deallocator;
    map->root = nullptr;
    map->count = 0;
    return map;
}

int treeMapCount(const TreeMap* const map) {
    return map->count;
}

int treeMapIteratorSize(const TreeMap* const map) {
    return (int) sizeof(TreeMapIterator) + (int) sizeof(Node*) * map->count;
}

static Node* nodeCreate(TreeMap* const map, const int key, void* const value) {
    Node* const node = xmalloc(sizeof *node);
    assert(node);

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
    xfree(node);
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
    Node* x = map->root, * y = nullptr, * new = nodeCreate(map, key, value);

    while (x) {
        y = x;
        if (key < x->key) x = x->left;
        else if (key > x->key) x = x->right;
        else return;
    }

    if (!y) map->root = new;
    else {
        if (key < y->key) y->left = new;
        else if (key > y->key) y->right = new;
        else assert(false);
        new->parent = y;
    }

    insertFixup(map, new);
}

static Node* nullable searchKey(TreeMap* const map, const int key) {
    Node* node = map->root;
    while (node) {
        if (key < node->key) node = node->left;
        else if (key > node->key) node = node->right;
        else return node;
    }
    return nullptr;
}

void* nullable treeMapSearchKey(TreeMap* const map, const int key) {
    Node* const node = searchKey(map, key);
    return node ? node->value : nullptr;
}

static void* nullable searchMinOrMax(Node* node, const bool minOrMax) {
    while (minOrMax ? node->left : node->right)
        minOrMax ? (node = node->left) : (node = node->right);
    return node;
}

void* nullable treeMapSearchMinOrMax(TreeMap* const map, const bool minOrMax) {
    return searchMinOrMax(map->root, minOrMax);
}

static void transplant(TreeMap* const map, Node* nullable const u, Node* nullable const v) {
    if (!parentOf(u)) map->root = v;
    else if (u == leftOf(parentOf(u))) setLeftOf(parentOf(u), v);
    else setRightOf(parentOf(u), v);
    setParentOf(v, parentOf(u));
}

static void deleteFixup(TreeMap* const map, Node* nullable x) {
    while (x != map->root && colorOf(x) == COLOR_BLACK) {
        if (x == leftOf(x->parent)) {
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
    Node* const found = searchKey(map, key);
    if (!found) return;

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
}

// TODO: assertions/invariants, thread-safety

static void iteratePutLeftChildrenIntoStack(TreeMapIterator* const iterator, Node* nullable node) {
    while (node) {
        assert(iterator->currentStackTop + 1 <= iterator->map->count);
        iterator->stack[++iterator->currentStackTop] = node;
        node = node->left;
    }
}

#undef treeMapIterateBegin
void treeMapIterateBegin(TreeMap* const map, TreeMapIterator* const iterator) {
    unconst(iterator->map) = map;
    iterator->currentStackTop = -1;
    xmemset(iterator->stack, 0, sizeof(Node*) * map->count);

    iteratePutLeftChildrenIntoStack(iterator, map->root);
}

void* nullable treeMapIterate(TreeMapIterator* const iterator) {
    if (iterator->currentStackTop < 0) return nullptr;

    Node* const node = iterator->stack[iterator->currentStackTop--];

    if (rightOf(node))
        iteratePutLeftChildrenIntoStack(iterator, node->right);

    return node;
}

void treeMapDestroy(TreeMap* const map) {
    TreeMapIterator* const iterator = xalloca2(treeMapIteratorSize(map));
    treeMapIterateBegin(map, iterator);

    Node* node;
    while ((node = treeMapIterate(iterator)))
        nodeDestroy(map, node);

    xfree(map);
}
