
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "lifecycle.h"
#include "net.h"

const int NET_ADDRESS_STRING_SIZE = 3 * 4 + 3 + 1; // xxx.xxx.xxx.xxx\n
static const short NET_LISTENER_SOCKET_PORT = 8080; // TODO: rename to listenerBroadcast

static atomic bool gInitialized = false;
static SDL_Mutex* gMutex = nullptr;

static List* gNetsList = nullptr; // <NetNet*>

static NetNet* gSelectedNet = nullptr;
static SDLNet_DatagramSocket* gNetListenerSocket = nullptr; // TODO: rename to gNetBroadcastListenerSocket
static atomic bool gListeningNet = false; // TODO: rename to listeningAndBroadcasting
static int gBroadcastTicker = 0;

void netInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    assert(SDLNet_Init());

    assert(gMutex = SDL_CreateMutex());

    gNetsList = listCreate(false, xfree);
}

bool netInitialized(void) {
    return gInitialized;
}

static void scanNets(void) {
    assert(lifecycleInitialized() && gInitialized);

    SDL_LockMutex(gMutex);
    listClear(gNetsList);

    struct ifaddrs* ifaddrRoot;
    assert(!getifaddrs(&ifaddrRoot));

    for (struct ifaddrs* ifaddr = ifaddrRoot; ifaddr; ifaddr = ifaddr->ifa_next) {
        if (ifaddr->ifa_addr->sa_family != AF_INET) continue;
        if ((ifaddr->ifa_flags & IFF_UP) != IFF_UP) continue;

        const unsigned hostAddress = swapBytes(*(unsigned*) (ifaddr->ifa_addr->sa_data + 2));
        const unsigned subnetMask = swapBytes(*(unsigned*) (ifaddr->ifa_netmask->sa_data + 2));
        const unsigned netAddress = hostAddress & subnetMask;
        const unsigned broadcastAddress = netAddress + ~subnetMask;

        byte mask = 0;
        for (unsigned n = subnetMask; n; n & 1 ? mask++ : 0, n >>= 1);

        if (hostAddress == 0x7f000001 || (ifaddr->ifa_flags & IFF_LOOPBACK) == IFF_LOOPBACK) continue;

        NetNet* const net = xmalloc(sizeof *net);
        assert(net);
        xmemcpy(net, &(NetNet) {
            {0},
            (int) netAddress,
            mask,
            (int) broadcastAddress,
            (int) hostAddress,
            (int) ((broadcastAddress - 1u) - (netAddress + 1u) + 1u),
            (netAddress & 0xff000000) == 0x0a000000 || (netAddress & 0xfff00000) == 0xac100000 || (netAddress & 0xffff0000) == 0xc0a80000, // private networks https://www.arin.net/reference/research/statistics/address_filters
            (ifaddr->ifa_flags & IFF_RUNNING) == IFF_RUNNING
        }, sizeof *net);
        strncpy((char*) net->name, ifaddr->ifa_name, sizeof(net->name));

        assert(hostAddress >= netAddress && hostAddress <= broadcastAddress);
        assert(net->hostsCount);

        listAddBack(gNetsList, net);
    }
    SDL_UnlockMutex(gMutex);

    freeifaddrs(ifaddrRoot);
}

static void* netDuplicator(const void* const old) {
    NetNet* const new = xmalloc(sizeof *new);
    assert(new);
    xmemcpy(new, old, sizeof *new);
    return new;
}

List* nullable netNets(void) {
    SDL_LockMutex(gMutex);
    List* const new = listCopy(gNetsList, false, netDuplicator);
    SDL_UnlockMutex(gMutex);
    return new;
}

void netAddressToString(char* const buffer, const int address) {
    assert(lifecycleInitialized() && gInitialized);
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

void netStartListeningNet(const NetNet* const net) { // TODO: rename to listeningAndBroadcasting
    assert(lifecycleInitialized() && gInitialized);

    SDL_LockMutex(gMutex);
    assert(!gSelectedNet && !gNetListenerSocket && !gListeningNet);

    gSelectedNet = netDuplicator(net);

    SDLNet_Address* const addr = resolveAddress(gSelectedNet->host);
    assert(gNetListenerSocket = SDLNet_CreateDatagramSocket(addr, NET_LISTENER_SOCKET_PORT));
    SDLNet_UnrefAddress(addr);

    assert(!setsockopt(
        *(int*) ((void*) gNetListenerSocket + 20),
        SOL_SOCKET,
        SO_BROADCAST,
        (int[1]){1},
        sizeof(int)
    ));

    gListeningNet = true;
    SDL_UnlockMutex(gMutex);
}

void netStopListeningNet(void) { // TODO: rename l & b
    assert(lifecycleInitialized() && gInitialized);

    SDL_LockMutex(gMutex);
    assert(gSelectedNet && gNetListenerSocket && gListeningNet);

    xfree(gSelectedNet);
    gSelectedNet = nullptr;

    SDLNet_DestroyDatagramSocket(gNetListenerSocket);
    gNetListenerSocket = nullptr;

    gListeningNet = false;
    SDL_UnlockMutex(gMutex);
}

static void broadcastNetForHosts(void) {
    SDL_LockMutex(gMutex);
    assert(gSelectedNet && gNetListenerSocket && gListeningNet);

    const int bufferSize = 1024;
    byte buffer[bufferSize] = {0}; // TODO

    SDLNet_Address* const addr = resolveAddress(gSelectedNet->broadcast); // can be changed to 0xffffffff (INADDR_BROADCAST) and therefore there's no need to use each subnet's mask and therefore there's no need to use getifaddrs as it can be replaced with SDLNet_GetLocalAddresses
    assert(SDLNet_SendDatagram(gNetListenerSocket, addr, NET_LISTENER_SOCKET_PORT, buffer, bufferSize));
    SDLNet_UnrefAddress(addr);

    SDL_UnlockMutex(gMutex);

    SDL_Log("broadcast");
}

void netLoop(void) {
    assert(lifecycleInitialized() && gInitialized);

    scanNets();

    if (gListeningNet && ++gBroadcastTicker == 100) {
        gBroadcastTicker = 0;
        broadcastNetForHosts();
    }
}

void netQuit(void) {
    assert(gInitialized);

    if (gListeningNet) netStopListeningNet();

    gInitialized = false;

    listDestroy(gNetsList);

    SDL_DestroyMutex(gMutex);

    SDLNet_Quit();
}
