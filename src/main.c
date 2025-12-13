
#include "collections/deque.h"
#include "lifecycle.h"

// TODO: how to safely destroy a locked mutex? - wait until released? how to ensure noone would access it after destroying/invalidating the object?

// const int argc, const char* const* const argv, const char* const* const envp
int main(void) {
    xmalloc(10);
    xmalloc(20);
    xfree(xmalloc(30));
    xmalloc(40);

    xrealloc(nullptr, 11);
    xrealloc(xmalloc(50), 52);
    xrealloc(xmalloc(60), 0);

    xfree(xrealloc(xmalloc(70), 73));

//    lifecycleInit();
//    lifecycleLoop();
//    lifecycleQuit();
    checkUnfreedAllocations();
    return 0;
}
