
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <sys/socket.h>
#include <netdb.h>
#include "lifecycle.h"
#include "crypto.h"
#include "net.h"

enum _NetMessageFlag : byte {
    NET_MESSAGE_FLAG_BROADCAST_HOST_DISCOVERY
};

typedef struct {} MessagePayload;

typedef struct packed {
    const MessagePayload;
    const byte signature[CRYPTO_SIGNATURE_SIZE];
    const char greeting[12];
    const byte version;
    const int address; // address of the sender
    const byte masterSealPublicKey[CRYPTO_ENCRYPT_PUBLIC_KEY_SIZE];
} HostDiscoveryBroadcastPayload;

const int NET_ADDRESS_STRING_SIZE = 3 * 4 + 3 + 1; // xxx.xxx.xxx.xxx\n
static const short NET_BROADCAST_SOCKET_PORT = 8080;
static const int BROADCAST_SUBNET_TICKER_PERIOD = 100;
static const int UDP_PACKET_MAX_SIZE = 512, BROADCAST_PAYLOAD_SIZE = UDP_PACKET_MAX_SIZE - (int) sizeof(NetMessage);
#define GREETING "Klenalo ping"

static atomic bool gInitialized = false;
static SDL_Mutex* gMutex = nullptr;

static List* gSubnetsHostsAddressesList = nullptr; // <int>

static atomic int gSelectedSubnetHostAddress = 0;
static SDLNet_DatagramSocket* gSubnetBroadcastSocket = nullptr;
static int gBroadcastSubnetTicker = 0;

static byte gHostDiscoveryBroadcastPayload[sizeof(HostDiscoveryBroadcastPayload)] = {0};

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

static void generateHostDiscoveryBroadcastPayload(void) {
    HostDiscoveryBroadcastPayload payload = {{}, {0}, GREETING, 1, gSelectedSubnetHostAddress, {0}};
    xmemcpy((byte*) payload.masterSealPublicKey, cryptoMasterSealPublicKey(), CRYPTO_ENCRYPT_PUBLIC_KEY_SIZE);
    cryptoMasterSign((void*) &payload + CRYPTO_SIGNATURE_SIZE, sizeof payload - CRYPTO_SIGNATURE_SIZE, (byte*) payload.signature);
    xmemcpy(gHostDiscoveryBroadcastPayload, &payload, sizeof payload);
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

    generateHostDiscoveryBroadcastPayload();

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

    const int bufferSize = 512;
    byte buffer[bufferSize] = {0}; // TODO

    SDLNet_Address* const address = resolveAddress(INADDR_BROADCAST);
    assert(SDLNet_SendDatagram(gSubnetBroadcastSocket, address, NET_BROADCAST_SOCKET_PORT, buffer, bufferSize));
    SDLNet_UnrefAddress(address);

    SDL_UnlockMutex(gMutex);

    SDL_Log("broadcast"); // TODO: test
}

void netLoop(void) {
    assert(lifecycleInitialized() && gInitialized);

    fetchSubnetsHostsAddresses();

    if (gSelectedSubnetHostAddress && ++gBroadcastSubnetTicker == BROADCAST_SUBNET_TICKER_PERIOD) {
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
