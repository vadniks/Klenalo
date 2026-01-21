
#include <stdlib.h>
#include <time.h>
#include "../src/collections/list.h"

static Compared listNoDynamicMemoryComparator(const void* const a, const void* const b) {
    return (long) *(void**) a < (long) *(void**) b
        ? COMPARED_LESS
        : (long) *(void**) a > (long) *(void**) b
            ? COMPARED_GREATER
            : COMPARED_EQUAL;
}

static void* listNoDynamicMemoryDuplicator(const void* const value) {
    return (void*) value;
}

static void listNoDynamicMemory(void) {
    const int amount = 10;
    List* const list = listCreate(DEFAULT_ALLOCATOR, false, nullptr);

    for (int i = 1; i <= amount; i++) {
        listAddBack(list, (void*) (long) i);
        assert(listGet(list, i - 1) == (void*) (long) i);
    }

    listClear(list);
    assert(!listSize(list));
    /////////////////

    for (int i = 1; i <= amount; i++)
        listAddFront(list, (void*) (long) i);
    for (int i = 1; i <= amount; i++)
        assert(listGet(list, i - 1) == (void*) (long) (amount - i + 1));

    /////////////////
    List* const list2 = listCopy(list, false, listNoDynamicMemoryDuplicator);
    assert(listSize(list) == listSize(list2));

    for (int i = 0; i < amount; i++)
        assert(listGet(list, i) == listGet(list2, i));

    listDestroy(list2);

    listClear(list);
    /////////////////

    for (int i = 1; i <= amount; i++)
        listAddBack(list, (void*) (long) i);

    int ii = 1, item, jj = amount;
    while ((item = (int) (long) listPopFirst(list))) {
        assert(item == ii++);
        assert(listSize(list) == --jj);
    }

    listClear(list);
    /////////////////

    for (int i = 1; i <= amount; i++)
        listAddBack(list, (void*) (long) i);

    ii = amount;
    while ((item = (int) (long) listPopLast(list))) {
        assert(item == ii--);
        assert(listSize(list) == ii);
    }

    listClear(list);
    /////////////////

    for (int i = 1; i <= amount; i++)
        listAddBack(list, (void*) (long) i);

    listRemove(list, 0);
    assert(listSize(list) == amount - 1);

    for (int i = 1; i <= amount - 1; i++)
        assert(listGet(list, i - 1) == (void*) (long) i + 1);

    listRemove(list, amount - 2);
    assert(listSize(list) == amount - 2);

    for (int i = 1; i <= amount - 2; i++)
        assert(listGet(list, i - 1) == (void*) (long) i + 1);

    ii = amount - 2;
    while (listSize(list)) {
        listRemove(list, 0);
        ii--;
    }
    assert(!ii);

    /////////////////
    assert(!listSize(list));

    listAddBack(list, (void*) 2ul);
    listAddFront(list, (void*) 1ul);
    listAddBack(list, (void*) 3ul);

    assert(listPeekFirst(list) == (void*) 1ul);
    assert(listPeekLast(list) == (void*) 3ul);

    listClear(list);
    /////////////////

    for (int i = 1; i <= amount; i++)
        listAddBack(list, (void*) (long) i);

    listSwap(list, 0, amount - 1);
    assert(listGet(list, 0) == (void*) (long) amount);
    assert(listGet(list, amount - 1) == (void*) (long) 1);

    /////////////////

    srand(time(nullptr));

    for (int i = 1; i <= amount; i++)
        listSwap(list, rand() % amount, rand() % amount);

    listQSort(list, listNoDynamicMemoryComparator);

    for (int i = 1, j = 0, k; i <= amount; i++) {
        k = (int) (long) listGet(list, i - 1);
        assert(k >= j);
        j = k;
    }

    /////////////////

    for (int value = 1; value <= amount; value++) {
        for (int index = 0; index < amount; index++) {
            void* const xvalue = (void*) (long) value;
            if (listGet(list, index) == xvalue)
                assert(listBinarySearch(list, &xvalue, listNoDynamicMemoryComparator) == xvalue);
        }
    }

    listDestroy(list);
}

void testCollections(void) {
    listNoDynamicMemory();
}
