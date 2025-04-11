
#include "lifecycle.h"

int main(void) {
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    assert(xallocations() == 0);
    return 0;
}
