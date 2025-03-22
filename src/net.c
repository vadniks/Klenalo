
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <sys/socket.h>
#include <netdb.h>
#include "lifecycle.h"
#include "crypto.h"
#include "consts.h"
#include "net.h"

enum _NetMessageFlag : byte {
    NET_MESSAGE_FLAG_BROADCAST_HOST_DISCOVERY
};

typedef struct packed {
    union {
        const NetMessagePayload;
        struct {
            const char greeting[12];
            const byte version;
            const byte masterSessionSealPublicKey[CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE];
            const byte wholeMessageSignature[CRYPTO_SIGNATURE_SIZE]; // signature not just of the payload but of the whole message and therefore it's recalculated everytime a new broadcast message is prepared to be sent
        };
    };
} HostDiscoveryBroadcastPayload;

static const short NET_BROADCAST_SOCKET_PORT = 8080;
static const int FETCH_SUBNETS_HOST_ADDRESSES_PERIOD = 100, SUBNET_BROADCAST_SEND_PERIOD = 1000, SUBNET_BROADCAST_RECEIVE_PERIOD = 250;
static const int UDP_PACKET_MAX_SIZE = 512, BROADCAST_PAYLOAD_SIZE = UDP_PACKET_MAX_SIZE - (int) sizeof(NetMessage);
static const int HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE = sizeof(HostDiscoveryBroadcastPayload);
#define GREETING constsConcatenateTitleWith(" ping")

static atomic bool gInitialized = false;
static SDL_Mutex* gMutex = nullptr;

static List* gSubnetsHostsAddressesList = nullptr; // <int>

static atomic int gSelectedSubnetHostAddress = 0;
static SDLNet_DatagramSocket* gSubnetBroadcastSocket = nullptr;

void netInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    assert(SDLNet_Init());

    assert(gMutex = SDL_CreateMutex());

    gSubnetsHostsAddressesList = listCreate(false, nullptr);
}

bool netInitialized(void) {
    return gInitialized;
}

static void fetchSubnetsHostsAddresses(void) {
    SDL_LockMutex(gMutex);
    listClear(gSubnetsHostsAddressesList);

    int numberOfAddresses = 0;
    SDLNet_Address** const addresses = SDLNet_GetLocalAddresses(&numberOfAddresses);
    assert(numberOfAddresses && addresses);

    for (int i = 0; i < numberOfAddresses; i++) {
        const struct addrinfo* const info = *(struct addrinfo**) ((void*) addresses[i] + 32);
        const int addr = (int) swapBytes(*(unsigned*) (info->ai_addr->sa_data + 2));

        if (info->ai_addr->sa_family != AF_INET) continue;
        if (addr == INADDR_LOOPBACK) continue;

        listAddBack(gSubnetsHostsAddressesList, (void*) (long) addr);
    }

    SDLNet_FreeLocalAddresses(addresses);
    SDL_UnlockMutex(gMutex);
}

List* nullable netSubnetsHostsAddresses(void) {
    SDL_LockMutex(gMutex);
    List* const new = listCopy(gSubnetsHostsAddressesList, false, nullptr);
    SDL_UnlockMutex(gMutex);
    return new;
}

void netAddressToString(char* const buffer, const int address) {
    assert(lifecycleInitialized() && gInitialized);
    xmemset(buffer, 0, NET_ADDRESS_STRING_SIZE);
    const byte count = SDL_snprintf(
        buffer,
        NET_ADDRESS_STRING_SIZE,
        "%u.%u.%u.%u",
        (address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff
    );
    assert(count > 0 && count < NET_ADDRESS_STRING_SIZE);
}

static SDLNet_Address* resolveAddress(const int address) {
    char host[NET_ADDRESS_STRING_SIZE];
    netAddressToString(host, address);

    SDLNet_Address* const resolvedAddress = SDLNet_ResolveHostname(host);
    assert(resolvedAddress);
    assert(SDLNet_WaitUntilResolved(resolvedAddress, -1) == 1);
    return resolvedAddress;
}

static void generateHostDiscoveryBroadcastPayload(HostDiscoveryBroadcastPayload* const payload) {
    strncpy((char*) payload->greeting, GREETING, sizeof payload->greeting);
    unconst(payload->version) = 1;
    xmemcpy((byte*) payload->masterSessionSealPublicKey, cryptoMasterSessionSealPublicKey(), CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE);
    xmemset((byte*) payload->wholeMessageSignature, 0, CRYPTO_SIGNATURE_SIZE);
}

void netStartBroadcastingAndListeningSubnet(const int subnetHostAddress) {
    assert(lifecycleInitialized() && gInitialized);
    assert(subnetHostAddress);

    SDL_LockMutex(gMutex);
    assert(!gSelectedSubnetHostAddress && !gSubnetBroadcastSocket);

    gSelectedSubnetHostAddress = subnetHostAddress;

    SDLNet_Address* const address = resolveAddress(gSelectedSubnetHostAddress);
    assert(gSubnetBroadcastSocket = SDLNet_CreateDatagramSocket(address, NET_BROADCAST_SOCKET_PORT));
    SDLNet_UnrefAddress(address);

    assert(!setsockopt(
        *(int*) ((void*) gSubnetBroadcastSocket + 20),
        SOL_SOCKET,
        SO_BROADCAST,
        (int[1]) {1},
        sizeof(int)
    ));

    SDL_UnlockMutex(gMutex);
}

void netStopBroadcastingAndListeningSubnet(void) {
    assert(lifecycleInitialized() && gInitialized);

    SDL_LockMutex(gMutex);
    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket);

    gSelectedSubnetHostAddress = 0;

    SDLNet_DestroyDatagramSocket(gSubnetBroadcastSocket);
    gSubnetBroadcastSocket = nullptr;

    SDL_UnlockMutex(gMutex);
}

static inline HostDiscoveryBroadcastPayload* hostDiscoveryBroadcastPayload(NetMessage* const message) {
    return (HostDiscoveryBroadcastPayload*) message->payload;
}

static bool a = false; // TODO: test only
static void broadcastSubnetForHosts(void) {
    const int messageSize = NET_MESSAGE_SIZE + HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE;
    staticAssert(messageSize <= UDP_PACKET_MAX_SIZE);

    NetMessage* const message = xalloca(messageSize);
    unconst(message->flag) = NET_MESSAGE_FLAG_BROADCAST_HOST_DISCOVERY;
    unconst(message->timestamp) = lifecycleCurrentTimeMillis();
    unconst(message->index) = 0;
    unconst(message->count) = 1;
    unconst(message->from) = gSelectedSubnetHostAddress;
    unconst(message->to) = NET_MESSAGE_TO_EVERYONE;
    unconst(message->size) = HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE;
    generateHostDiscoveryBroadcastPayload(hostDiscoveryBroadcastPayload(message));
    cryptoMasterSign(
        (byte*) message,
        messageSize - CRYPTO_SIGNATURE_SIZE,
        (byte*) hostDiscoveryBroadcastPayload(message)->wholeMessageSignature
    );

    SDLNet_Address* const address = resolveAddress(!a ? gSelectedSubnetHostAddress /*TODO: test only*/: message->to);
    a = true; // TODO: test only

    SDL_LockMutex(gMutex);

    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket);
    assert(message->from == gSelectedSubnetHostAddress);
    assert(SDLNet_SendDatagram(gSubnetBroadcastSocket, address, NET_BROADCAST_SOCKET_PORT, message, messageSize));

    SDL_UnlockMutex(gMutex);

    SDLNet_UnrefAddress(address);
}

static void listenSubnetForBroadcasts(void) {
    // TODO: recfrom(...socket...)

    SDLNet_Datagram* datagram;

    SDL_LockMutex(gMutex);
    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket);
    assert(SDLNet_ReceiveDatagram(gSubnetBroadcastSocket, &datagram));
    SDL_UnlockMutex(gMutex);

    if (datagram) {
        debugArgs("%s:%u %d", SDLNet_GetAddressString(datagram->addr), datagram->port, datagram->buflen)
        printMemory(datagram->buf, datagram->buflen, PRINT_MEMORY_MODE_TRY_STR_HEX_FALLBACK);

        if (datagram->buflen == NET_MESSAGE_SIZE + HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE) // as anyone can send anything anywhere over udp
            assert(cryptoCheckMasterSigned(datagram->buf, datagram->buflen - CRYPTO_SIGNATURE_SIZE, datagram->buf + (datagram->buflen - CRYPTO_SIGNATURE_SIZE)));

        SDLNet_DestroyDatagram(datagram);
    }
}

static void runPeriodically(const unsigned long currentMillis, unsigned long* const lastRunMillis, const int period, void (* const action)(void)) {
    if (currentMillis - *lastRunMillis < (unsigned) period) return;
    *lastRunMillis = currentMillis;
    action();
}

void netLoop(void) {
    assert(lifecycleInitialized() && gInitialized);

    const unsigned long currentMillis = lifecycleCurrentTimeMillis();
    static unsigned long lastSubnetsHostsAddressesFetch = 0, lastBroadcastSend = 0, lastBroadcastReceive = 0;

    runPeriodically(currentMillis, &lastSubnetsHostsAddressesFetch, FETCH_SUBNETS_HOST_ADDRESSES_PERIOD, fetchSubnetsHostsAddresses);

    if (gSelectedSubnetHostAddress) {
        runPeriodically(currentMillis, &lastBroadcastSend, SUBNET_BROADCAST_SEND_PERIOD, broadcastSubnetForHosts);
        runPeriodically(currentMillis, &lastBroadcastReceive, SUBNET_BROADCAST_RECEIVE_PERIOD, listenSubnetForBroadcasts);
    }
}

void netQuit(void) {
    assert(gInitialized);

    if (gSelectedSubnetHostAddress) netStopBroadcastingAndListeningSubnet();

    gInitialized = false;

    listDestroy(gSubnetsHostsAddressesList);

    SDL_DestroyMutex(gMutex);

    SDLNet_Quit();
}
