
#include "lifecycle.h"

int main(void) {
    lifecycleInit();
    lifecycleLoop();
    lifecycleQuit();
    return 0;
}
