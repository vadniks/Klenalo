
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <sys/socket.h>
#include <netdb.h>
#include "lifecycle.h"
#include "net.h"

const int NET_ADDRESS_STRING_SIZE = 3 * 4 + 3 + 1; // xxx.xxx.xxx.xxx\n
static const short NET_BROADCAST_SOCKET_PORT = 8080;

static atomic bool gInitialized = false;
static SDL_Mutex* gMutex = nullptr;

static List* gSubnetsHostsAddressesList = nullptr; // <int>

static atomic int gSelectedSubnetHostAddress = 0;
static SDLNet_DatagramSocket* gSubnetBroadcastSocket = nullptr;
static int gBroadcastTicker = 0;

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
    assert(lifecycleInitialized() && gInitialized);

    SDL_LockMutex(gMutex);
    listClear(gSubnetsHostsAddressesList);

    int numAddrs = 0;
    SDLNet_Address** const addrs = SDLNet_GetLocalAddresses(&numAddrs);
    assert(numAddrs && addrs);

    for (int i = 0; i < numAddrs; i++) {
        const struct addrinfo* const info = *(struct addrinfo**) ((void*) addrs[i] + 32);
        const int addr = (int) swapBytes(*(unsigned*) (info->ai_addr->sa_data + 2));

        if (info->ai_addr->sa_family != AF_INET) continue;
        if (addr == INADDR_LOOPBACK) continue;

        listAddBack(gSubnetsHostsAddressesList, (void*) (long) addr);
    }

    SDLNet_FreeLocalAddresses(addrs);
    SDL_UnlockMutex(gMutex);
}

static void* subnetHostAddressDuplicator(const void* const address) { // TODO: make itemDuplicator optional in List
    return (void*) address; // addresses themselves are values
}

List* nullable netSubnetsHostsAddresses(void) {
    SDL_LockMutex(gMutex);
    List* const new = listCopy(gSubnetsHostsAddressesList, false, subnetHostAddressDuplicator);
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

    SDLNet_Address* const addr = SDLNet_ResolveHostname(host);
    assert(addr);
    assert(SDLNet_WaitUntilResolved(addr, -1) == 1);
    return addr;
}

void netStartBroadcastingAndListeningSubnet(const int subnetHostAddress) {
    assert(lifecycleInitialized() && gInitialized);
    assert(subnetHostAddress);

    SDL_LockMutex(gMutex);
    assert(!gSelectedSubnetHostAddress && !gSubnetBroadcastSocket);

    gSelectedSubnetHostAddress = subnetHostAddress;

    SDLNet_Address* const addr = resolveAddress(gSelectedSubnetHostAddress);
    assert(gSubnetBroadcastSocket = SDLNet_CreateDatagramSocket(addr, NET_BROADCAST_SOCKET_PORT));
    SDLNet_UnrefAddress(addr);

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

static void broadcastSubnetForHosts(void) {
    SDL_LockMutex(gMutex);
    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket);

    const int bufferSize = 1024;
    byte buffer[bufferSize] = {0}; // TODO

    SDLNet_Address* const addr = resolveAddress(INADDR_BROADCAST);
    assert(SDLNet_SendDatagram(gSubnetBroadcastSocket, addr, NET_BROADCAST_SOCKET_PORT, buffer, bufferSize));
    SDLNet_UnrefAddress(addr);

    SDL_UnlockMutex(gMutex);

    SDL_Log("broadcast"); // TODO: test
}

void netLoop(void) {
    assert(lifecycleInitialized() && gInitialized);

    fetchSubnetsHostsAddresses();

    if (gSelectedSubnetHostAddress && ++gBroadcastTicker == /*TODO: extract*/100) {
        gBroadcastTicker = 0;
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
