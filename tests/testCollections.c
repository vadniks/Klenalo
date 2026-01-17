
#include "../src/collections/list.h"

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

    // TODO:

    listDestroy(list);
}

void testCollections(void) {
    listNoDynamicMemory();
}
