
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

TreeMap* treeMapCreate(const TreeMapDeallocator nullable deallocator) {
    TreeMap* const map = xmalloc(sizeof *map);
    assert(map);
    unconst(map->deallocator) = deallocator;
    map->root = nullptr;
    map->count = 0;
    return map;
}

static inline void deallocateValue(const TreeMap* const map, void* const value) {
    if (map->deallocator) map->deallocator(value);
}

static Node* nodeCreate(const int key, void* const value) {
    Node* const node = xmalloc(sizeof *node);
    assert(node);
    unconst(node->key) = key;
    unconst(node->value) = value;
    node->parent = node->left = node->right = nullptr;
    node->color = COLOR_RED;
    return node;
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

static void insertFixup(TreeMap* const map, Node* z) {
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
    Node* x = map->root, * y = nullptr, * z = nodeCreate(key, value);

    while (x) {
        y = x;
        if (key < x->key) x = x->left;
        else if (key > x->key) x = x->right;
        else assert(false);
    }

    if (!y) map->root = z;
    else {
        if (key < y->key) y->left = z;
        else if (key > y->key) y->right = z;
        else assert(false);
        z->parent = y;
    }

    insertFixup(map, z);
}

//void treeMapInsert(TreeMap* const map, const int key, void* const value) {
//    if (!map->root) {
//        map->root = nodeCreate(key, value);
//        map->count = 1;
//        return;
//    }
//
//    Node* node = map->root, * parent;
//    char comparison;
//
//    do {
//        parent = node;
//        if (key < node->key) {
//            node = node->left;
//            comparison = -1;
//        } else if (key > node->key) {
//            node = node->right;
//            comparison = 1;
//        } else assert(false);
//    } while (node);
//
//    node = nodeCreate(key, value);
//    comparison == -1 ? (parent->left = node) : (parent->right = node);
//    fixAfterInsert(map, node);
//    map->count++;
//}

void* nullable treeMapSearchKey(TreeMap* const map, const int key) {
    Node* node = map->root;
    while (node) {
        if (key < node->key) node = node->left;
        else if (key > node->key) node = node->right;
        else return node->value;
    }
    return nullptr;
}

void* nullable treeMapSearchMinOrMax(TreeMap* const map, const bool minOrMax) {
    Node* node = map->root;
    while (node) minOrMax ? (node = node->left) : (node = node->right);
    return node;
}

void treeMapDelete(TreeMap* const map, const int key) {

}

static void destroyNodes(TreeMap* const map, Node* const root) {
    if (!root) return;

    destroyNodes(map, root->left);
    destroyNodes(map, root->right);

    deallocateValue(map, root->value);
    xfree(root);
}

void treeMapDestroy(TreeMap* const map) {
    destroyNodes(map, map->root);
    xfree(map);
}
