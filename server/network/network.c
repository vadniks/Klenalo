
#include <SDL3_net/SDL_net.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include "../../common/collections/treemap.h"
#include "../../common/collections/vector.h"
#include "../core/lifecycle.h"
#include "network.h"

enum {
    ADDRESS_NAME_SIZE = 16
};

typedef struct {
    char name[ADDRESS_NAME_SIZE];
    struct in6_addr address;
    byte mask;
} Address;

static atomic bool gInitialized = false;

void networkInit() {
    assert(!gInitialized && lifecycleInitialized());
    gInitialized = true;

    assert(NET_Init());
}

bool networkInitialized() {
    return gInitialized;
}

static Vector* /* Address* */ queryInterfaces() {
    struct ifaddrs* addrs;
    assert(!getifaddrs(&addrs));

    Treemap* const addressesMap = treemapCreate(nullptr); // <Address*>

    for (const struct ifaddrs* interface = addrs; interface; interface = interface->ifa_next) {
        if (!(interface->ifa_flags & IFF_UP) ||
            !(interface->ifa_flags & IFF_RUNNING) ||
            interface->ifa_flags & IFF_LOOPBACK) continue;

        const struct sockaddr* const interfaceAddress = interface->ifa_addr;

        if (interfaceAddress->sa_family == AF_PACKET) {
            Address* const address = malloc(sizeof *address);
            strncpy(address->name, interface->ifa_name, sizeof address->name);
            address->mask = 0;

            treemapInsert(addressesMap, hashValue(address->name, sizeof address->name), address);
        } else if (interfaceAddress->sa_family == AF_INET6) {
            Address* const ourAddress = treemapSearchKey(addressesMap, hashValue(interface->ifa_name, sizeof ourAddress->name));

            const struct in6_addr addr = ((struct sockaddr_in6*) (void*) interface->ifa_addr)->sin6_addr;
            const struct in6_addr netmask = ((struct sockaddr_in6*) (void*) interface->ifa_netmask)->sin6_addr;

            byte mask = 0;
            for (int j = 0; j < (int) sizeof netmask.__in6_u.__u6_addr8; j++)
                mask += popcount(netmask.__in6_u.__u6_addr8[j]);

            // not a link-local address and has mask of first 64 (default) bits set to ones
            if ((addr.__in6_u.__u6_addr32[0] & bswap(0xffc00000)) != bswap(0xfe800000) && mask == 64) {
                ourAddress->address = addr;
                ourAddress->mask = mask;
            }
        }
    }
    freeifaddrs(addrs);

    Vector* const addressesVector = vectorCreate(free); // <Address*>

    TreemapIterator* iterator;
    treemapIterateBegin(addressesMap, iterator);
    Address* address;
    while ((address = treemapIterate(iterator))) {
        printf("%s ", address->name);
        printMemory(&address->address, 16, PRINT_MEMORY_MODE_IPV6, false);
        printf(" ");
        printf("%d\n", address->mask);

        vectorAddBack(addressesVector, address);
    }
    treemapIterateEnd(iterator);

    treemapDestroy(addressesMap);
    return addressesVector;
}

static void queryInterfaces0() {
    Vector* const addresses = queryInterfaces();
    for (int i = 0; i < vectorSize(addresses); i++) {
        const Address* const address = vectorGet(addresses, i);
        printMemory(&address->address, 16, PRINT_MEMORY_MODE_IPV6, true);
    }
    vectorDestroy(addresses);
}

static void runPeriodically(
    const unsigned long currentMillis, // TODO: replace unsigned long with ulong/u64 everywhere
    unsigned long* const lastRunMillis,
    const int periodMillis,
    void (* const action)(void)
) {
    if (currentMillis - *lastRunMillis < (unsigned) periodMillis) return;
    *lastRunMillis = currentMillis;
    action();
}

void networkLoopBody() {
    static unsigned long lastSubnetsQuery = 0;
    runPeriodically(currentTimeMillis(), &lastSubnetsQuery, 3000, queryInterfaces0);
}

void networkQuit() {
    assert(gInitialized);
    gInitialized = false;

    NET_Quit();
}
