
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
static const int BROADCAST_SUBNET_TICKER_PERIOD = 100;
static const int UDP_PACKET_MAX_SIZE = 512, BROADCAST_PAYLOAD_SIZE = UDP_PACKET_MAX_SIZE - (int) sizeof(NetMessage);
static const int HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE = sizeof(HostDiscoveryBroadcastPayload);
#define GREETING constsConcatenateTitleWith(" ping")

static atomic bool gInitialized = false;
static SDL_Mutex* gMutex = nullptr;

static List* gSubnetsHostsAddressesList = nullptr; // <int>

static atomic int gSelectedSubnetHostAddress = 0;
static SDLNet_DatagramSocket* gSubnetBroadcastSocket = nullptr;
static int gBroadcastSubnetTicker = 0;

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

    // TODO: recfrom(...socket...)

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

static void broadcastSubnetForHosts(void) {
    SDL_LockMutex(gMutex);
    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket);

    const int messageSize = NET_MESSAGE_SIZE + HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE;
    staticAssert(messageSize <= UDP_PACKET_MAX_SIZE);

    NetMessage* const message = xAlloca(messageSize);
    unconst(message->flag) = 0;
    unconst(message->timestamp) = lifecycleCurrentTimeMillis();
    unconst(message->index) = 0;
    unconst(message->count) = 1;
    unconst(message->from) = gSelectedSubnetHostAddress;
    unconst(message->to) = INADDR_BROADCAST;
    unconst(message->size) = HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE;
    generateHostDiscoveryBroadcastPayload((HostDiscoveryBroadcastPayload*) message->payload);
    cryptoMasterSign(
        (byte*) message,
        messageSize - CRYPTO_SIGNATURE_SIZE,
        (byte*) ((HostDiscoveryBroadcastPayload*) message->payload)->wholeMessageSignature
    );

    // TODO: test only
    printMemory(message, messageSize, PRINT_MEMORY_MODE_TRY_STR_HEX_FALLBACK);
    assert(cryptoCheckMasterSigned((byte*) message, messageSize - CRYPTO_SIGNATURE_SIZE, (void*) message + (messageSize - CRYPTO_SIGNATURE_SIZE)));
    assert(!xmemcmp(message->payload, ((HostDiscoveryBroadcastPayload*) message->payload)->payload, HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE));
    assert(memmem(((HostDiscoveryBroadcastPayload*) message->payload)->payload, HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE, ((HostDiscoveryBroadcastPayload*) message->payload)->greeting, sizeof((HostDiscoveryBroadcastPayload){}.greeting)));

    SDLNet_Address* const address = resolveAddress(INADDR_BROADCAST);
    assert(SDLNet_SendDatagram(gSubnetBroadcastSocket, address, NET_BROADCAST_SOCKET_PORT, message, messageSize));
    SDLNet_UnrefAddress(address);

    SDL_UnlockMutex(gMutex);
}

void netLoop(void) {
    assert(lifecycleInitialized() && gInitialized);

    fetchSubnetsHostsAddresses();

    if (gSelectedSubnetHostAddress && ++gBroadcastSubnetTicker == BROADCAST_SUBNET_TICKER_PERIOD) { // TODO: replace with time based ticker
        gBroadcastSubnetTicker = 0;
        broadcastSubnetForHosts();
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
