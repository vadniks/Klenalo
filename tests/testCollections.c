
#include <stdlib.h>
#include "../src/collections/list.h"

static const int LIST_ITEMS_AMOUNT = 10;
static List* gList = nullptr;
static bool gNdm; // no dynamic memory

static void init(void) {
    gList = listCreate(DEFAULT_ALLOCATOR, false, gNdm ? nullptr : xfree);
}

inline static void* newValue(const int value) {
    void* buffer;
    if (gNdm) (buffer = (void*) (long) value);
    else (buffer = xmalloc(sizeof(int))) && (*(int*) buffer = value);
    return buffer;
}

static void addBack(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++) {
        void* const value = newValue(i);
        listAddBack(gList, value);
        assert(listGet(gList, i - 1) == value);
    }

    listClear(gList);
    assert(!listSize(gList));
}

inline static int valueToInt(const void* const value) {
    return gNdm ? (int) (long) value : *(int*) value;
}

static void addFront(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listAddFront(gList, newValue(i));
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        assert(valueToInt(listGet(gList, i - 1)) == LIST_ITEMS_AMOUNT - i + 1);
}

static void* valueDuplicator(void* const value) {
    return gNdm ? value : newValue(*(int*) value);
}

static void copy(void) {
    List* const list2 = listCopy(gList, false, valueDuplicator);
    assert(listSize(gList) == listSize(list2));

    for (int i = 0; i < LIST_ITEMS_AMOUNT; i++)
        assert(valueToInt(listGet(gList, i)) == valueToInt(listGet(list2, i)));

    listDestroy(list2);
    listClear(gList);
}

inline static void removeIfDM(void* const value) {
    if (!gNdm) xfree(value);
}

static void popFirst(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listAddBack(gList, newValue(i));

    void* item;
    int i = 1, j = LIST_ITEMS_AMOUNT;
    while ((item = listPopFirst(gList))) {
        assert(valueToInt(item) == i++);
        removeIfDM(item);
        assert(listSize(gList) == --j);
    }

    listClear(gList);
}

static void popLast(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listAddBack(gList, newValue(i));

    void* item;
    int i = LIST_ITEMS_AMOUNT;
    while ((item = listPopLast(gList))) {
        assert(valueToInt(item) == i--);
        removeIfDM(item);
        assert(listSize(gList) == i);
    }

    listClear(gList);
}

static void remove(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listAddBack(gList, newValue(i));

    listRemove(gList, 0);
    assert(listSize(gList) == LIST_ITEMS_AMOUNT - 1);

    for (int i = 1; i <= LIST_ITEMS_AMOUNT - 1; i++)
        assert(valueToInt(listGet(gList, i - 1)) == i + 1);

    listRemove(gList, LIST_ITEMS_AMOUNT - 2);
    assert(listSize(gList) == LIST_ITEMS_AMOUNT - 2);

    for (int i = 1; i <= LIST_ITEMS_AMOUNT - 2; i++)
        assert(valueToInt(listGet(gList, i - 1)) == i + 1);

    int i = LIST_ITEMS_AMOUNT - 2;
    while (listSize(gList)) {
        listRemove(gList, 0);
        i--;
    }
    assert(!i);

    assert(!listSize(gList));
}

static void peek(void) {
    listAddBack(gList, newValue(2));
    listAddFront(gList, newValue(1));
    listAddBack(gList, newValue(3));

    assert(valueToInt(listPeekFirst(gList)) == 1);
    assert(valueToInt(listPeekLast(gList)) == 3);

    listClear(gList);
}

static void swap(void) {
    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listAddBack(gList, newValue(i));

    listSwap(gList, 0, LIST_ITEMS_AMOUNT - 1);
    assert(valueToInt(listGet(gList, 0)) == LIST_ITEMS_AMOUNT);
    assert(valueToInt(listGet(gList, LIST_ITEMS_AMOUNT - 1)) == 1);
}

static Compared comparator(const void* const a, const void* const b) {
    const int aa = gNdm ? (int) (long) *(void**) a : **(int**) a;
    const int bb = gNdm ? (int) (long) *(void**) b : **(int**) b;

    if (aa < bb) return COMPARED_LESS;
    if (aa > bb) return COMPARED_GREATER;
    return COMPARED_EQUAL;
}

static void sort(void) {
    long time(long* const);
    srand((unsigned) time(nullptr)); // NOLINT(*-msc51-cpp)

    for (int i = 1; i <= LIST_ITEMS_AMOUNT; i++)
        listSwap(gList, rand() % LIST_ITEMS_AMOUNT, rand() % LIST_ITEMS_AMOUNT); // NOLINT(*-msc50-cpp)

    listQSort(gList, comparator);

    for (int i = 1, j = 0, k; i <= LIST_ITEMS_AMOUNT; i++) {
        k = valueToInt(listGet(gList, i - 1));
        assert(k >= j);
        j = k;
    }
}

static void binarySearch(void) {
    for (int i = 0; i < LIST_ITEMS_AMOUNT; i++) {
        void* const value = listGet(gList, i);
        assert(listBinarySearch(gList, &value, comparator) == value);
    }
}

static void quit(void) {
    listDestroy(gList);
}

void testCollectionsList(void) {
    gNdm = 1;

    round:
    init();
    addBack();
    addFront();
    copy();
    popFirst();
    popLast();
    remove();
    peek();
    swap();
    sort();
    binarySearch();
    quit();

    if (gNdm--) goto round;
}
