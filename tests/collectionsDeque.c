
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

static void popOutermost(void) {

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
    quit();

    if (gNdm--) goto round;
}
