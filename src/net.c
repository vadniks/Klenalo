
#include <SDL3/SDL.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "lifecycle.h"
#include "net.h"

const int NET_ADDRESS_STRING_SIZE = 3 * 4 + 3 + 1; // xxx.xxx.xxx.xxx\n

static atomic bool gInitialized = false;

static struct {
    List* list; // <NetNet*>
    SDL_Mutex* mutex;
} gNetsList = {nullptr, nullptr};

void netInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    gNetsList.list = listCreate(false, xfree);
    assert(gNetsList.mutex = SDL_CreateMutex());
}

bool netInitialized(void) {
    return gInitialized;
}

static void scanNets(void) {
    assert(lifecycleInitialized() && gInitialized);

    SDL_LockMutex(gNetsList.mutex);
    listClear(gNetsList.list);

    struct ifaddrs* ifaddrRoot;
    assert(!getifaddrs(&ifaddrRoot));

    for (struct ifaddrs* ifaddr = ifaddrRoot; ifaddr; ifaddr = ifaddr->ifa_next) {
        if (ifaddr->ifa_addr->sa_family != AF_INET) continue;

        const unsigned hostAddress = swapBytes(*(unsigned*) (ifaddr->ifa_addr->sa_data + 2));
        const unsigned subnetMask = swapBytes(*(unsigned*) (ifaddr->ifa_netmask->sa_data + 2));
        const unsigned netAddress = hostAddress & subnetMask;
        const unsigned broadcastAddress = netAddress + ~subnetMask;

        byte mask = 0;
        for (unsigned n = subnetMask; n; n & 1 ? mask++ : STUB, n >>= 1);

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

        listAddBack(gNetsList.list, net);
    }
    SDL_UnlockMutex(gNetsList.mutex);

    freeifaddrs(ifaddrRoot);
}

static void* netDuplicator(const void* const old) {
    NetNet* const new = xmalloc(sizeof *new);
    assert(new);
    xmemcpy(new, old, sizeof *new);
    return new;
}

List* nullable netNets(void) {
    SDL_LockMutex(gNetsList.mutex);
    List* const new = listCopy(gNetsList.list, false, netDuplicator);
    SDL_UnlockMutex(gNetsList.mutex);
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

static void ping(void) {


}

void netListen(void) {
    assert(lifecycleInitialized() && gInitialized);
    scanNets();
}

void netQuit(void) {
    assert(gInitialized);
    gInitialized = false;

    SDL_DestroyMutex(gNetsList.mutex);
    listDestroy(gNetsList.list);
}
