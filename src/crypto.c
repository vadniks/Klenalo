
#include <sodium.h>
#include "lifecycle.h"
#include "crypto.h"

static atomic bool gInitialized = false;

void cryptoInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    assert(sodium_init() >= 0);
}

bool cryptoInitialized(void) {
    return gInitialized;
}

void cryptoMasterSign(const byte* const message, const int size, byte* const buffer) {

}

void cryptoQuit(void) {
    assert(gInitialized);
    gInitialized = false;
}
