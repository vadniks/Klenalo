
#include "../src/collections/deque.h"

static const int ITEMS_AMOUNT = 10;
static Deque* gDeque = nullptr;
static bool gNdm; // no dynamic memory

static void init(void) {
    gDeque = dequeCreate(DEFAULT_ALLOCATOR, false, gNdm ? nullptr : xfree);
}

inline static void* newValue(const int value) {
    if (gNdm) return (void*) (long) value;
    void* const buffer = xmalloc(sizeof(int));
    *(int*) buffer = value;
    return buffer;
}

inline static int valueToInt(const void* const value) {
    return gNdm ? (int) (long) value : *(int*) value;
}

static void pushBack(void) {
    for (int i = 1; i <= ITEMS_AMOUNT; i++)
        dequePushBack(gDeque, newValue(i));
    for (int i = 0; i < ITEMS_AMOUNT; i++)
        assert(valueToInt(dequeGet(gDeque, i, true)) == i + 1);

    dequeClear(gDeque);
    assert(!dequeSize(gDeque));
}

static void pushFront(void) {
    for (int i = 1; i <= ITEMS_AMOUNT; i++)
        dequePushFront(gDeque, newValue(i));
    for (int i = 0, j = ITEMS_AMOUNT; i < ITEMS_AMOUNT; i++, j--)
        assert(valueToInt(dequeGet(gDeque, i, true)) == j);
}

inline static void removeIfDM(void* const value) { // TODO: reame to freeIfDM
    if (!gNdm) xfree(value);
}

static void popOutermost(void) {
    // after pushFront the order is reversed

    void* value = dequePopFirst(gDeque);
    assert(valueToInt(value) == ITEMS_AMOUNT);
    removeIfDM(value);

    value = dequePopLast(gDeque);
    assert(valueToInt(value) == 1);
    removeIfDM(value);

    assert(valueToInt(dequeGet(gDeque, 3, true)) == 6);
    assert(valueToInt(dequeGet(gDeque, 4, true)) == ITEMS_AMOUNT / 2);

    value = dequePopLast(gDeque);
    assert(valueToInt(value) == 2);
    removeIfDM(value);

    value = dequePopFirst(gDeque);
    assert(valueToInt(value) == ITEMS_AMOUNT - 1);
    removeIfDM(value);
}

static void popFirst(void) {
    dequeClear(gDeque);

    for (int i = 1; i <= ITEMS_AMOUNT; i++)
        dequePushBack(gDeque, newValue(i));

    int value = 1;
    void* temp;
    while ((temp = dequePopFirst(gDeque))) {
        assert(valueToInt(temp) == value++);
        removeIfDM(temp);
    }

    assert(!dequeSize(gDeque));
}

static void popLast(void) {
    dequeClear(gDeque);

    for (int i = 1; i <= ITEMS_AMOUNT; i++)
        dequePushBack(gDeque, newValue(i));

    int value = ITEMS_AMOUNT;
    void* temp;
    while ((temp = dequePopLast(gDeque))) {
        assert(valueToInt(temp) == value--);
        removeIfDM(temp);
    }

    assert(!dequeSize(gDeque));
}

static void remove(void) {
    dequeClear(gDeque);
    for (int i = 1; i <= ITEMS_AMOUNT; i++)
        dequePushBack(gDeque, newValue(i));

    dequeRemove(gDeque, 0);
    assert(dequeSize(gDeque) == ITEMS_AMOUNT - 1);
    for (int i = 0; i < ITEMS_AMOUNT - 1; i++)
        assert(valueToInt(dequeGet(gDeque, i, false)) == i + 2);

    dequeRemove(gDeque, ITEMS_AMOUNT - 2);
    assert(dequeSize(gDeque) == ITEMS_AMOUNT - 2);
    for (int i = 0; i < ITEMS_AMOUNT - 2; i++)
        assert(valueToInt(dequeGet(gDeque, i, false)) == i + 2);
}

static void removeInTheMiddle(void) {
    dequeClear(gDeque);

    for (int i = 1; i <= ITEMS_AMOUNT; i++)
        dequePushBack(gDeque, newValue(i));

    dequeRemove(gDeque, 4);
    dequeRemove(gDeque, 4);

    assert(valueToInt(dequeGet(gDeque, 0, true)) == 1);
    assert(valueToInt(dequeGet(gDeque, 1, true)) == 2);
    assert(valueToInt(dequeGet(gDeque, 2, true)) == 3);
    assert(valueToInt(dequeGet(gDeque, 3, true)) == 4);
    assert(valueToInt(dequeGet(gDeque, 4, true)) == 7);
    assert(valueToInt(dequeGet(gDeque, 5, true)) == 8);
    assert(valueToInt(dequeGet(gDeque, 6, true)) == 9);
    assert(valueToInt(dequeGet(gDeque, 7, true)) == 10);

    assert(dequeSize(gDeque) == ITEMS_AMOUNT - 2);
}

static void removeOutermostAndPush(void) {
    dequeClear(gDeque);

    for (int i = 1; i <= ITEMS_AMOUNT; i++)
        dequePushBack(gDeque, newValue(i));

    dequeRemove(gDeque, 0);

    for (int i = 0; i < ITEMS_AMOUNT - 1; i++)
        assert(valueToInt(dequeGet(gDeque, i, true)) == i + 2);

    dequeRemove(gDeque, ITEMS_AMOUNT - 2);

    for (int i = 0; i < ITEMS_AMOUNT - 2; i++)
        assert(valueToInt(dequeGet(gDeque, i, false)) == i + 2);

    const int n1 = 100;
    dequePushBack(gDeque, newValue(n1));

    for (int i = 0; i < ITEMS_AMOUNT - 2; i++)
        assert(valueToInt(dequeGet(gDeque, i, true)) == i + 2);

    assert(valueToInt(dequeGet(gDeque, ITEMS_AMOUNT - 2, false)) == n1);

    const int n2 = 200;
    dequePushFront(gDeque, newValue(n2));

    for (int i = 1; i < ITEMS_AMOUNT - 1; i++)
        assert(valueToInt(dequeGet(gDeque, i, true)) == i + 1);

    assert(valueToInt(dequeGet(gDeque, ITEMS_AMOUNT - 1, false)) == n1);
    assert(valueToInt(dequeGet(gDeque, 0, false)) == n2);
}

static void peek(void) {
    dequeClear(gDeque);

    for (int i = 1; i <= ITEMS_AMOUNT; i++)
        dequePushBack(gDeque, newValue(i));

    assert(valueToInt(dequePeekFirst(gDeque)) == 1);
    assert(valueToInt(dequePeekLast(gDeque)) == ITEMS_AMOUNT);
}

static void quit(void) {
    dequeDestroy(gDeque);
}

void testCollectionsDeque(void) {
    gNdm = 1;

    round:
    init();
    pushBack();
    pushFront();
    popOutermost();
    popFirst();
    popLast();
    remove();
    removeInTheMiddle();
    removeOutermostAndPush();
    peek();
    quit();

    if (gNdm--) goto round;
}
