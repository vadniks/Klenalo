
#include "shared.h"
#include "network.h"
#include "discovery.h"

typedef struct {
    long magic;
    byte version;
    vec16aw address;
    // cryptographic signature
} HelloPacket;

static atomic bool gInitialized = false;
static NET_DatagramSocket* gServerSocket = nullptr; // listens for others
static NET_DatagramSocket* gClientSocket = nullptr; // pings others

void discoveryInit() {
    assert(!gInitialized && networkInitialized());
    gInitialized = true;

    struct addrinfo* ai;
    assert(!getaddrinfo("", nullptr, nullptr, &ai)); // leaky

    NET_Address* const address = malloc(sizeof *address);
    *address = (NET_Address) {
        nullptr,
        nullptr,
        nullptr,
        {0},
        NET_SUCCESS,
        ai,
        nullptr
    };

    const SDL_PropertiesID props = SDL_CreateProperties();
    assert(props);
    assert(SDL_SetBooleanProperty(props, NET_PROP_DATAGRAM_SOCKET_ALLOW_BROADCAST_BOOLEAN, true));

    // TODO: we actually don't need all that complexity, we can choose interface from SDL_Net's functionality as we don't need to actually calculate subnetting

    assert(gServerSocket = NET_CreateDatagramSocket(address, 8080, props)); // leaky
    SDL_DestroyProperties(props);
    NET_UnrefAddress(address);

    NET_Datagram* datagram = nullptr;
    int a = 0;
    do {
        // assert(NET_ReceiveDatagram(gServerSocket, &datagram));
    } while (datagram && ++a < 10);

    if (datagram) printMemory(datagram, sizeof *datagram, PRINT_MEMORY_MODE_HEXDUMP, true);
    printf("a\n");
}

void discoveryScan(const Address* const host) {

}

void discoveryQuit() {
    assert(gInitialized);
    gInitialized = false;

    NET_DestroyDatagramSocket(gServerSocket);
}
