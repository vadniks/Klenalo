
#include <SDL2/SDL_stdinc.h>
#include "redBlackTree.h"

// https://www.geeksforgeeks.org/introduction-to-red-black-tree/

typedef enum {RED, BLACK} Color;

typedef struct _Node {
    int key;
    void* value;
    Color color;
    struct _Node* left, * parent, * right;
} Node;

struct _RedBlackTree {
    Node* root;
    RedBlackTreeDeallocator nullable deallocator;
};

static const int MAX_SIZE = 0x7fffffff;

static const Node* const NIL = (void*) 0xfffffffffffffffful;

RedBlackTree* redBlackTreeInit(const RedBlackTreeDeallocator nullable deallocator) {
    RedBlackTree* const redBlackTree = SDL_malloc(sizeof *redBlackTree);
    assert(redBlackTree);
    redBlackTree->root = nullptr;
    redBlackTree->deallocator = deallocator;
    return redBlackTree;
}

static void rotateLeft(RedBlackTree* const redBlackTree, Node* const x) {
    Node* const y = x->right;

    x->right = y->left;
    if (y->left != NIL) y->left->parent = x;

    y->parent = x->parent;
    if (!x->parent) redBlackTree->root = y;
    else if (x == x->parent->left) x->parent->left = y;
    else x->parent->right = y;

    y->left = x;
    x->parent = y;
}

static void rotateRight(Node* const node) {

}

static void fixInsert(Node* const node) {

}

void redBlackTreeInsert(RedBlackTree* const redBlackTree, const int key, void* const value) {

}

void* nullable redBlackTreeGet(const RedBlackTree* const redBlackTree, const int key) {

}

void redBlackTreeDestroy(RedBlackTree* const redBlackTree) {

}
