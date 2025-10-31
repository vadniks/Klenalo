
#include "avltree.h"

typedef struct Node {
    const void* const value;
    struct Node* nullable left;
    struct Node* nullable right;
    int height;
} Node;

struct _Avltree {

};

struct _AvltreeTraverser {

};

const int AVLTREE_TRAVERSER_SIZE = sizeof(AvltreeTraverser);

Avltree* avltreeCreate(const bool synchronized) {
    return nullptr;
}

void avltreeInsert(Avltree* const tree, const void* const value) {

}

overloadable void* nullable avltreeSearch(Avltree* const tree, const void* const key, const AvltreeComparator comparator) {
    return nullptr;
}

overloadable void* nullable avltreeSearch(Avltree* const tree, const bool minOrMax, const AvltreeComparator comparator) {
    return nullptr;
}

void avltreeDelete(Avltree* const tree, const void* const value) {

}

#undef avltreeTraverseBegin
void avltreeTraverseBegin(Avltree* const tree, AvltreeTraverser* const traverser) {

}

void* nullable avlTreeTraverse(AvltreeTraverser* const traverser) {
    return nullptr;
}

void avltreeTraverseEnd(AvltreeTraverser* const traverser) {

}

void avltreeDestroy(Avltree* const tree) {

}
