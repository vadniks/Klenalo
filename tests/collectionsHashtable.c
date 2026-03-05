
#include "../src/collections/hashtable.h"

static const int ITEMS_AMOUNT = 10;
static Hashtable* gHashtable = nullptr;
static bool gNdm; // no dynamic memory

static void init(void) {
    gHashtable = nullptr; //hashtableCreate(DEFAULT_ALLOCATOR, false, gNdm ? nullptr : xfree);
}

inline static void* newValue(const int value) {
    void* buffer;
    if (gNdm) (buffer = (void*) (long) value);
    else (buffer = xmalloc(sizeof(int))) && (*(int*) buffer = value);
    return buffer;
}

inline static int valueToInt(const void* const value) {
    return gNdm ? (int) (long) value : *(int*) value;
}

static void put(void) {
    for (byte i = 1; i <= ITEMS_AMOUNT; i++)
//        printf("%d %d\n", i, hashValue(&i, 1));
        hashtablePut(gHashtable, hashValue(&i, 1), newValue(i));

    HashtableIterator* iterator;
    hashtableIterateBegin(gHashtable, iterator);
    void* value;
    byte i = 1;
    while ((value = hashtableIterate(iterator)))
        assert(valueToInt(value) == i++);
//        printf("%d\n", valueToInt(value));
    hashtableIterateEnd(iterator);
}

static void quit(void) {
    hashtableDestroy(gHashtable);
}

void testCollectionsHashtable(void) {
    gNdm = 1;

    round:
//    init();
//    put();
//
//    quit();

//    if (gNdm--) goto round;
}
