
#include "rwMutex.h"
#include "avltree.h"

// inspired by https://www.w3schools.com/dsa/dsa_data_avltrees.php

struct _AvltreeNode {
    const void* const value;
    AvltreeNode
        * nullable left,
        * nullable right;
    int height;
};

struct _Avltree {
    const AvltreeComparator comparator;
    RWMutex* nullable const rwMutex;
    const AvltreeDeallocator nullable deallocator;
    int elements;
};

struct _AvltreeTraverser {

};

const int AVLTREE_TRAVERSER_SIZE = sizeof(AvltreeTraverser);

Avltree* avltreeCreate(const AvltreeComparator comparator, const bool synchronized, const AvltreeDeallocator nullable deallocator) {
    Avltree* const tree = xmalloc(sizeof *tree);
    assert(tree);
    unconst(tree->comparator) = comparator;
    unconst(tree->rwMutex) = synchronized ? rwMutexCreate() : nullptr;
    unconst(tree->deallocator) = deallocator;
    tree->elements = 0;
    return tree;
}

static inline void xRwMutexCommand(Avltree* const tree, const RWMutexCommand command) {
    if (tree->rwMutex) rwMutexCommand(tree->rwMutex, command);
}

int avltreeElements(Avltree* const tree) {
    return tree->elements;
}

static inline int nodeHeight(AvltreeNode* nullable const node) {
    return node ? node->height : 0;
}

static inline int nodeBalance(AvltreeNode* nullable const node) {
    return node ? nodeHeight(node->left) - nodeHeight(node->right) : 0;
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "UnreachableCode" // bug
static AvltreeNode* nodeRotate(AvltreeNode* const node, const bool rightOrLeft) { // true=right, false=left
    AvltreeNode* const target = rightOrLeft ? node->left : node->right;

    AvltreeNode* const temp = rightOrLeft ? node->right : node->left;
    rightOrLeft ? (target->right = node) : (target->left = node);
    rightOrLeft ? (node->left = temp) : (node->right = temp);

    node->height = 1 + max(nodeHeight(node->left), nodeHeight(node->right));
    target->height = 1 + max(nodeHeight(target->left), nodeHeight(target->right));

    printf("%s %d | %d %d\n", rightOrLeft ? "right" : "left", *(int*) node->value, node->height, target->height);

    assert(target);
    return target;
}
#pragma clang diagnostic pop

static AvltreeNode* createNode(const void* const value) {
    AvltreeNode* const node = xmalloc(sizeof *node);
    assert(node);
    unconst(node->value) = value;
    node->left = node->right = nullptr;
    node->height = 1;
    return node;
}

AvltreeNode* avltreeInsert(Avltree* const tree, AvltreeNode* nullable node, const void* const value) {
    if (!node) {
        node = createNode(value);
        tree->elements++;
        return node;
    }

    if (tree->comparator(value, node->value) == -1) // left < right
        node->left = avltreeInsert(tree, node->left, value);
    else if (tree->comparator(value, node->value) == 1) // left > right
        node->right = avltreeInsert(tree, node->right, value);
    else
        assert(false);

    node->height = 1 + max(nodeHeight(node->left), nodeHeight(node->right));
    const int balance = nodeBalance(node);

//    if (balance > 1) {
//        if (nodeBalance(node->left) < 0)
//            node->left = nodeRotate(node->left, false);
//        return nodeRotate(node, true);
//    } else if (balance < -1) {
//        if (nodeBalance(node->right) > 0)
//            node->right = nodeRotate(node->right, true);
//        return nodeRotate(node, false);
//    } else
//        return node;

    if (balance > 1 && nodeBalance(node->left) >= 0)
        return nodeRotate(node, true);

    if (balance > 1 && nodeBalance(node->left) < 0) {
        node->left = nodeRotate(node->left, false);
        return nodeRotate(node, true);
    }

    if (balance < -1 && nodeBalance(node->right) <= 0)
        return nodeRotate(node, false);

    if (balance < -1 && nodeBalance(node->right) > 0) {
        node->right = nodeRotate(node->right, true);
        return nodeRotate(node, false);
    }

    return node;
}

void* nullable avltreeSearchKey(Avltree* const tree, const void* const key) {
    return nullptr;
}

void* nullable avltreeSearchMinOrMax(Avltree* const tree, const bool minOrMax) {
    return nullptr;
}

void avltreeDelete(Avltree* const tree, const void* const value) {

}

void avltreeTraverse(Avltree* const tree, const AvltreeNode* nullable const node, const AvltreeVisitor visitor) {
    if (!node) return;
    avltreeTraverse(tree, node->left, visitor);
    visitor(node->value);
    avltreeTraverse(tree, node->right, visitor);
}

#undef avltreeTraverseBegin
void avltreeTraverseBegin(Avltree* const tree, AvltreeTraverser* const traverser) {

}

void* nullable avltreeTraverseLinearly(AvltreeTraverser* const traverser) {
    return nullptr;
}

void avltreeTraverseEnd(AvltreeTraverser* const traverser) {

}

static void destroy(Avltree* const tree, AvltreeNode* nullable const node) {
    if (!node) return;

    destroy(tree, node->left);
    destroy(tree, node->right);

    if (tree->deallocator) tree->deallocator((void*) node->value);
    xfree(node);
}

void avltreeDestroy(Avltree* const tree, AvltreeNode* nullable const node) {
    if (tree->rwMutex) rwMutexDestroy(tree->rwMutex);
    destroy(tree, node);
    xfree(tree);
}
