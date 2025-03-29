
#include "lifecycle.h"

int main(void) {
    const unsigned long allocations = xallocations();
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    assert(xallocations() == allocations);
    return 0;
}
