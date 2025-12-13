
#include "../collections/deque.h"
#include "lifecycle.h"

// TODO: how to safely destroy a locked mutex? - wait until released? how to ensure noone would access it after destroying/invalidating the object?

// const int argc, const char* const* const argv, const char* const* const envp
int main(void) {
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    checkUnfreedAllocations();
    return 0;
}
