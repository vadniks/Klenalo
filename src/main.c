
#include "collections/treeMap.h"
#include "lifecycle.h"

// TODO: how to safely destroy a locked mutex? - wait until released? how to ensure noone would access it after destroying/invalidating the object?

static int c = 0;

static void b(void);

#include <stdio.h>
#include <execinfo.h>
#include <string.h>

void _start(void);

noinline static void a(void) {
    if (c++ > 1) return;

    const void* const aEnd = memmem(a, 1024, "footer"/*(byte[]) {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff}*/, 6);

    const int size = 0xf;
    void* array[size];

    int recursion = -1;

    const int actualSize = backtrace(array, size);
    for (int i = 3; i < actualSize; i++) {
        if (array[i] > a && array[i] < aEnd) {
            recursion = i;
            break;
        }
    }

    printf("a=%p b=%p ret=%p aEnd=%p recursion=%d caller=%p\n", a, b, __builtin_return_address(0), aEnd, recursion, recursion >= 0 ? array[recursion] : 0);
    puts("");
    fprintf(stderr, "\n");

    // malloc
    b();

    asm volatile (
        "jmp .aEnd\n"
        ".ascii \"footer\"\n"
//        ".byte 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff\n"
        ".aEnd:\n"
    );
}

noinline static void b(void) {
    // create
    a();
    puts("@");
}

// const int argc, const char* const* const argv, const char* const* const envp
int main(void) {
//    auto startHooks = START_HOOKS;
//    void (* startHook)(void);
//    while ((startHook = *(startHooks += sizeof(startHook))))
//        startHook();

    if (START_HOOK) START_HOOK();
//    lifecycleInit();
//    lifecycleLoop();
//    lifecycleQuit();

//    a();

    TreeMap* const map = treeMapCreate(false, xfree);

    const int size = 11;
    const int array[size] = //{1, 5, 0, 8, 9, 7, 3, 2, 4, 6};
        {73913442, 9269314, -55374814, -120018942, -184663070, -249307198, -313951326, -378595454, 961417123, 896772995, 832128867};

    for (int i = 0; i < size; i++) {
        int* const value = xmalloc(sizeof(int));
        *value = array[i];
        treeMapInsert(map, array[i], value);
    }

    const int arr2[size] = {-378595454, -313951326, -249307198, -184663070, -120018942, -55374814, 9269314, 73913442, 832128867, 896772995, 961417123};

    treeMapDelete(map, 9);

    TreeMapIterator* iterator;
    treeMapIterateBegin(map, iterator);
    int* value;
    int a = 0;
    while ((value = treeMapIterate(iterator)))
        assert(*value == arr2[a++]);
//        printf("%d\n", *value);
    treeMapIterateEnd(iterator);

    treeMapDestroy(map);

    if (xallocations()) {
        xunfreedAllocations();
//        assert(false);
    }

    if (END_HOOK) END_HOOK();

    return 0;
}
