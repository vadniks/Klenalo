
#include "lifecycle.h"

int main(const int argc, const char* const* const argv, const char* const* const envp) {
    USED(argc), USED(argv), USED(envp);
//    lifecycleInit();
//    lifecycleLoop();
//    lifecycleQuit();
    assert(xallocations() == 0);
    return 0;
}
