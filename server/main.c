
#include "core/lifecycle.h"

// const int argc, const char* const* const argv, const char* const* const envp
int main() {
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    return 0;
}
