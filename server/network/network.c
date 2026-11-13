
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

typedef union {
    struct in6_addr scalar;
    vec16uw vector;
} In6Addr;

typedef struct {
    char interface[IF_NAMESIZE];
    unsigned interfaceIndex;
    In6Addr
        slaacAddress, // for user to see
        linkLocalAddress; // for us to use
    unsigned mask;
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

static typed(Vector*, Address*) queryInterfaces() {
    struct ifaddrs* addrs;
    assert(!getifaddrs(&addrs));

    typed(Treemap*, Address*) const addressesMap = treemapCreate(nullptr);

    for (const struct ifaddrs* interface = addrs; interface; interface = interface->ifa_next) {
        if (!(interface->ifa_flags & IFF_UP) ||
            !(interface->ifa_flags & IFF_RUNNING) ||
            interface->ifa_flags & IFF_LOOPBACK) continue;

        const struct sockaddr* const interfaceAddress = interface->ifa_addr;

        if (interfaceAddress->sa_family == AF_PACKET) {
            Address* const address = malloc(sizeof *address);
            strncpy(address->interface, interface->ifa_name, sizeof address->interface);
            address->interfaceIndex = if_nametoindex(address->interface);
            address->mask = 0;

            treemapInsert(addressesMap, hashValue(address->interface, sizeof address->interface), address);
        } else if (interfaceAddress->sa_family == AF_INET6) {
            Address* const address = treemapSearchKey(addressesMap, hashValue(interface->ifa_name, sizeof address->interface));

            const struct in6_addr addr = ((struct sockaddr_in6*) (void*) interface->ifa_addr)->sin6_addr;
            const struct in6_addr netmask = ((struct sockaddr_in6*) (void*) interface->ifa_netmask)->sin6_addr;

            byte mask = 0;
            for (int j = 0; j < (int) sizeof netmask.__in6_u.__u6_addr8; j++)
                mask += popcount(netmask.__in6_u.__u6_addr8[j]);

            // not a link-local address
            if ((addr.__in6_u.__u6_addr32[0] & bswap(0xffc00000)) != bswap(0xfe800000)) {
                // not a global routable address (DHCPv6 issued, hence - no mask/subnetPrefix - defaults to mask_max and no derivation from MAC address)
                if (mask < 128) {
                    address->slaacAddress.scalar = addr;
                    address->mask = mask;
                }
            } else {
                address->linkLocalAddress.scalar = addr;
                assert(address->mask == mask);
            }
        }
    }
    freeifaddrs(addrs);

    typed(Vector*, Address*) const addressesVector = vectorCreate(free);

    TreemapIterator* iterator;
    treemapIterateBegin(addressesMap, iterator);
    Address* address;
    while ((address = treemapIterate(iterator)))
        vectorAddBack(addressesVector, address);
    treemapIterateEnd(iterator);

    treemapDestroy(addressesMap);
    return addressesVector;
}

static void queryInterfaces0() {
    Vector* const addresses = queryInterfaces();
    for (int i = 0; i < vectorSize(addresses); i++) {
        const Address* const address = vectorGet(addresses, i);
        printf("%s: ", address->interface);
        printMemory(&address->slaacAddress, 16, PRINT_MEMORY_MODE_IPV6, false);
        printf("/%d", address->mask);
        printf(" = ");
        printMemory(&address->linkLocalAddress, 16, PRINT_MEMORY_MODE_IPV6, false);
        printf("%%%u\n", address->interfaceIndex);
    }
    vectorDestroy(addresses);
}

static void runPeriodically(
    const ulong currentMillis,
    ulong* const lastRunMillis,
    const int periodMillis,
    void (* const action)(void)
) {
    if (currentMillis - *lastRunMillis < (unsigned) periodMillis) return;
    *lastRunMillis = currentMillis;
    action();
}

void networkLoopBody() {
    static ulong lastSubnetsQuery = 0;
    runPeriodically(currentTimeMillis(), &lastSubnetsQuery, 3000, queryInterfaces0);
}

void networkQuit() {
    assert(gInitialized);
    gInitialized = false;

    NET_Quit();
}
