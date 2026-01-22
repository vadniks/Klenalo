
#include <stdlib.h>
#include <time.h>
#include "../src/collections/list.h"

static const int LIST_ITEMS_AMOUNT = 10;
static List* gList = nullptr;

static void ndmInit(void) { // no dynamic memory
    gList = listCreate(DEFAULT_ALLOCATOR, false, nullptr);
}

static void ndmAddBack(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++) {
        listAddBack(gList, (void*) (long) i);
        assert(listGet(gList, i - 1) == (void*) (long) i);
    }

    listClear(gList);
    assert(!listSize(gList));
}

static void ndmAddFront(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listAddFront(gList, (void*) (long) i);
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        assert(listGet(gList, i - 1) == (void*) (long) (LIST_ITEMS_AMOUNT - i + 1));
}

static void* ndmDuplicator(const void* const value) {
    return (void*) value;
}

static void ndmCopy(void) {
    List* const list2 = listCopy(gList, false, ndmDuplicator);
    assert(listSize(gList) == listSize(list2));

    for (int i = 0; i < LIST_ITEMS_AMOUNT; i++)
        assert(listGet(gList, i) == listGet(list2, i));

    listDestroy(list2);
    listClear(gList);
}

static void ndmPopFirst(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listAddBack(gList, (void*) (long) i);

    int i = 1, item, j = LIST_ITEMS_AMOUNT;
    while ((item = (int) (long) listPopFirst(gList))) {
        assert(item == i++);
        assert(listSize(gList) == --j);
    }

    listClear(gList);
}

static void ndmPopLast(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listAddBack(gList, (void*) (long) i);

    int i = LIST_ITEMS_AMOUNT, item;
    while ((item = (int) (long) listPopLast(gList))) {
        assert(item == i--);
        assert(listSize(gList) == i);
    }

    listClear(gList);
}

static void ndmRemove(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listAddBack(gList, (void*) (long) i);

    listRemove(gList, 0);
    assert(listSize(gList) == LIST_ITEMS_AMOUNT - 1);

    for (int i = 1; i <= LIST_ITEMS_AMOUNT - 1; i++)
        assert(listGet(gList, i - 1) == (void*) (long) i + 1);

    listRemove(gList, LIST_ITEMS_AMOUNT - 2);
    assert(listSize(gList) == LIST_ITEMS_AMOUNT - 2);

    for (int i = 1; i <= LIST_ITEMS_AMOUNT - 2; i++)
        assert(listGet(gList, i - 1) == (void*) (long) i + 1);

    int i = LIST_ITEMS_AMOUNT - 2;
    while (listSize(gList)) {
        listRemove(gList, 0);
        i--;
    }
    assert(!i);

    assert(!listSize(gList));
}

static void ndmPeek(void) {
    listAddBack(gList, (void*) 2ul);
    listAddFront(gList, (void*) 1ul);
    listAddBack(gList, (void*) 3ul);

    assert(listPeekFirst(gList) == (void*) 1ul);
    assert(listPeekLast(gList) == (void*) 3ul);

    listClear(gList);
}

static void ndmSwap(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listAddBack(gList, (void*) (long) i);

    listSwap(gList, 0, LIST_ITEMS_AMOUNT - 1);
    assert(listGet(gList, 0) == (void*) (long) LIST_ITEMS_AMOUNT);
    assert(listGet(gList, LIST_ITEMS_AMOUNT - 1) == (void*) (long) 1);
}

static Compared ndmComparator(const void* const a, const void* const b) {
    return (long) *(void**) a < (long) *(void**) b
        ? COMPARED_LESS
        : (long) *(void**) a > (long) *(void**) b
            ? COMPARED_GREATER
            : COMPARED_EQUAL;
}

static void ndmSort(void) {
    srand(time(nullptr));

    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listSwap(gList, rand() % LIST_ITEMS_AMOUNT, rand() % LIST_ITEMS_AMOUNT);

    listQSort(gList, ndmComparator);

    for (int i = 1, j = 0, k; i <= LIST_ITEMS_AMOUNT; i++) {
        k = (int) (long) listGet(gList, i - 1);
        assert(k >= j);
        j = k;
    }
}

static void ndmBinarySearch(void) {
    for (int value = 1; value <= LIST_ITEMS_AMOUNT; value++) {
        for (int index = 0; index < LIST_ITEMS_AMOUNT; index++) {
            void* const xvalue = (void*) (long) value;
            if (listGet(gList, index) == xvalue)
                assert(listBinarySearch(gList, &xvalue, ndmComparator) == xvalue);
        }
    }
}

static void ndmQuit(void) {
    listDestroy(gList);
}

void testCollectionsList(void) {
    ndmInit();
    ndmAddBack();
    ndmAddFront();
    ndmCopy();
    ndmPopFirst();
    ndmPopLast();
    ndmRemove();
    ndmPeek();
    ndmSwap();
    ndmSort();
    ndmBinarySearch();
    ndmQuit();
}
