
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

void discoveryInit(const Address* const host) {
    assert(!gInitialized && networkInitialized());
    gInitialized = true;

    const auto address = (NET_Address) {
        nullptr,
        nullptr,
        nullptr,
        {1}, // so sdlNet won't try to free this
        NET_SUCCESS,
        &(struct addrinfo) {
            AI_ADDRCONFIG,
            AF_INET6,
            SOCK_RAW,
            IPPROTO_IP,
            sizeof(struct sockaddr_in6),
            (struct sockaddr*) &(struct sockaddr_in6) {
                AF_INET6,
                0,
                0,
                *(struct in6_addr*) &host->linkLocal,
                host->interfaceIndex
            },
            nullptr,
            nullptr
        },
        nullptr
    };

    const SDL_PropertiesID props = SDL_CreateProperties();
    assert(props);
    assert(SDL_SetBooleanProperty(props, NET_PROP_DATAGRAM_SOCKET_ALLOW_BROADCAST_BOOLEAN, true));

    assert(gServerSocket = NET_CreateDatagramSocket((void*) &address, 8080, props)); // leaky
    SDL_DestroyProperties(props);

    NET_Datagram* datagram = nullptr;
    int a = 0;
    do {
        assert(NET_ReceiveDatagram(gServerSocket, &datagram));
        if (datagram) printf("b\n");
    } while (datagram && ++a < 10);

    if (datagram) printMemory(datagram, sizeof *datagram, PRINT_MEMORY_MODE_HEXDUMP, true);
    printf("a\n");
}

void discoveryBroadcast() {

}

void discoveryQuit() {
    assert(gInitialized);
    gInitialized = false;

    NET_DestroyDatagramSocket(gServerSocket);
}
