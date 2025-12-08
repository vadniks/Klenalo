
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

    // 0x555541b58ab3
    // 0x55564228

    const int size = 10;
    const int array[size] = {1, 5, 0, 8, 9, 7, 3, 2, 4, 6};

    for (int i = 0; i < size; i++) {
        int* const value = xmalloc(sizeof(int));
        *value = array[i];
        treeMapInsert(map, array[i], value);
    }

    for (int i = 0; i < size; i++)
        assert(*(int*) treeMapSearchKey(map, i) == i);

    treeMapDelete(map, 9);

    TreeMapIterator* iterator;
    treeMapIterateBegin(map, iterator);
    int* value;
    while ((value = treeMapIterate(iterator)))
        printf("%d\n", *value);
    treeMapIterateEnd(iterator);

    treeMapDestroy(map);

    if (END_HOOK) END_HOOK();
//    assert(xallocations() == 0);
    return 0;
}
