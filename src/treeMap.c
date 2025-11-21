
#include "treeMap.h"

struct _TreeMapNode {
    const TreeMapComparator comparator;
    const TreeMapDeallocator deallocator;
    void* const value;
    TreeMapNode
        * nullable left,
        * nullable right;
    int height;
};

static inline int height(TreeMapNode* nullable const node) {
    return node ? node->height : 0;
}

static inline int balance(TreeMapNode* nullable const node) {
    return node ? height(node->left) - height(node->right) : 0;
}

static TreeMapNode* rotate(TreeMapNode* const node, const bool rightOrLeft) { // true=right, false=left
    TreeMapNode* const target = rightOrLeft ? node->left : node->right;

    TreeMapNode* const temp = rightOrLeft ? node->right : node->left;
    rightOrLeft ? (target->right = node) : (target->left = node);
    rightOrLeft ? (node->left = temp) : (node->right = temp);

    node->height = 1 + max(height(node->left), height(node->right));
    target->height = 1 + max(height(target->left), height(target->right));

    assert(target);
    return target;
}

TreeMapNode* treeMapInsert(TreeMapNode* nullable head, void* const value, const TreeMapComparator nullable comparator, const TreeMapDeallocator nullable deallocator) {
    if (!head) {
        assert(comparator);

        head = xmalloc(sizeof *head);
        assert(head);

        unconst(head->comparator) = comparator;
        unconst(head->deallocator) = deallocator;
        unconst(head->value) = value;
        head->left = head->right = nullptr;
        head->height = 0;

        return head;
    }

    if (head->comparator(value, head->value) == -1) // left < right
        head->left = treeMapInsert(head->left, value, head->comparator, head->deallocator);
    else if (head->comparator(value, head->value) == 1) // left > right
        head->right = treeMapInsert(head->right, value, head->comparator, head->deallocator);
    else
        assert(false);

    head->height = 1 + max(height(head->left), height(head->right));
    const int xbalance = balance(head);

    if (xbalance > 1) {
        if (balance(head->left) < 0)
            head->left = rotate(head->left, false);
        return rotate(head, true);
    } else if (xbalance < -1) {
        if (balance(head->right) > 0)
            head->right = rotate(head->right, true);
        return rotate(head, false);
    } else
        return head;
}

void* nullable treeMapSearchKey(TreeMapNode* const head, const void* const key) {
    return nullptr;
}

void* nullable treeMapSearchMinOrMax(TreeMapNode* const head, const bool minOrMax) {
    return nullptr;
}

void treeMapDelete(TreeMapNode* const head, const void* const value) {
    assert(head->comparator(head->value, value) != 0);
}

static void traverse(const TreeMapNode* nullable const head, const TreeMapVisitor visitor) {
    if (!head) return;
    traverse(head->left, visitor);
    visitor(head->value);
    traverse(head->right, visitor);
}

void treeMapTraverse(const TreeMapNode* const head, const TreeMapVisitor visitor) {
    traverse(head, visitor);
}

static void destroy(TreeMapNode* nullable const head) {
    if (!head) return;

    destroy(head->left);
    destroy(head->right);

    head->deallocator(head->value);
    xfree(head);
}

void treeMapDestroy(TreeMapNode* const head) {
    destroy(head);
}

//

static int compare(const void* const a, const void* const b) {
    const int aa = *(int*) a, bb = *(int*) b;
    return aa < bb ? -1 : aa > bb ? 1 : 0;
}

static void visit(const void* const value) {
    putsf("%d", *(int*) value);
}

[[gnu::constructor(1)]]
static void test(void) {
    const int count = 20;
    const int values[count] = {10, -1, 4, -8, -10, -7, 9, 6, -9, 5, -5, 0, -3, 8, -4, 2, 7, 1, 3, -6};

    TreeMapNode* head = nullptr;
    for (int i = 0; i < count; i++) {
        int* const value = xmalloc(sizeof(int));
        assert(value);
        *value = values[i];

        head = treeMapInsert(nullptr, value, compare, xfree);
    }

    treeMapTraverse(head, visit);

    treeMapDestroy(head);
}
