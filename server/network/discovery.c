
#include "shared.h"
#include "network.h"
#include "discovery.h"

static atomic bool gInitialized = false;
static NET_DatagramSocket* gServerSocket = nullptr; // listens for others
static NET_DatagramSocket* gClientSocket = nullptr; // pings others

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
