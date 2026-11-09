
#include "../core/lifecycle.h"
#include "network.h"

static atomic bool gInitialized = false;

void networkInit() {
    assert(!gInitialized && lifecycleInitialized());
    gInitialized = true;
}

void networkLoopBody() {

}

void networkQuit() {
    assert(gInitialized);
    gInitialized = false;
}
