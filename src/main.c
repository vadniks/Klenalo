
#include "collections/deque.h"
#include "lifecycle.h"

// TODO: how to safely destroy a locked mutex? - wait until released? how to ensure noone would access it after destroying/invalidating the object?

// const int argc, const char* const* const argv, const char* const* const envp
int main(void) {
    Deque* const deque = dequeCreate(false, xfree);

    int* value;

    value = xmalloc(sizeof(int));
    *value = 0;
    dequePushFront(deque, value);

    value = xmalloc(sizeof(int));
    *value = 1;
    dequePushBack(deque, value);

    value = dequePopLast(deque);
    printf("a %d\n", *value);
    xfree(value);

    // TODO: add asserts (invariants checking) to public functions in Deck

    value = dequePopLast(deque);
    printf("a %d\n", *value);
    xfree(value);

    value = dequePopFirst(deque);
    printf("a %d\n", value ? *value : -1);
    xfree(value);

    while ((value = dequePopFirst(deque))) {
        printf("%d\n", *value);
        xfree(value);
    }

    printf("%p %p\n", dequeGet(deque, 5, true), dequeGet(deque, 5, false));

    dequeDestroy(deque);

//    lifecycleInit();
//    lifecycleLoop();
//    lifecycleQuit();
    assert(!xallocations());
    return 0;
}
