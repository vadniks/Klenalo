
#include "collections/deque.h"
#include "lifecycle.h"

// TODO: how to safely destroy a locked mutex? - wait until released? how to ensure noone would access it after destroying/invalidating the object?

// const int argc, const char* const* const argv, const char* const* const envp
int main(void) {
    Deque* const deque = dequeCreate(false, xfree);

    for (int i = 0; i < 10; i++) {
        int* const value = xmalloc(sizeof(int));
        *value = i;
        dequePushBack(deque, value);
    }

    int* value;
    while ((value = dequePopLast(deque))) {
        printf("%d\n", *value);
        xfree(value);
    }

    dequeDestroy(deque);

//    lifecycleInit();
//    lifecycleLoop();
//    lifecycleQuit();
    assert(!xallocations());
    return 0;
}
