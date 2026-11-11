
#include <SDL3_net/SDL_net.h>
#include "../../common/defs.h"
#include "network.h"
#include "discovery.h"

static atomic bool gInitialized = false;
static NET_DatagramSocket* gDiscoveryServerSocket = nullptr;
static NET_DatagramSocket* gDiscoveryClientSocket = nullptr;

void discoveryInit() {
    assert(!gInitialized && networkInitialized());
    gInitialized = true;

    // NET_CreateDatagramSocket()
}

void discoveryBroadcast() {

}

void discoveryQuit() {
    assert(gInitialized);
    gInitialized = false;
}
