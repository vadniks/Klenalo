
#include "collections/list.h"
#include "lifecycle.h"

// TODO: how to safely destroy a locked mutex? - wait until released? how to ensure noone would access it after destroying/invalidating the object?

// const int argc, const char* const* const argv, const char* const* const envp
int main(void) {
    List* const list = listCreate(false, xfree);
    // 7 of 11

    const int size = 11;

    for (int i = 0; i < size; i++) {
        int* const value = xmalloc(sizeof(int));
        *value = i;
        listAddBack(list, value);
    }

    listRemove(list, 10);

    for (int i = 0; i < listSize(list); i++)
        printf("%d\n", *(int*) listGet(list, i));

    listDestroy(list);

//    lifecycleInit();
//    lifecycleLoop();
//    lifecycleQuit();
    assert(!xallocations());
    return 0;
}
