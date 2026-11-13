
#include "../../common/collections/treemap.h"
#include "../../common/collections/vector.h"
#include "../core/lifecycle.h"
#include "shared.h"
#include "network.h"

static atomic bool gInitialized = false;
static Address gChosenAddress = {0}; // this host's address and interface index

void networkInit() {
    assert(!gInitialized && lifecycleInitialized());
    gInitialized = true;

    assert(NET_Init());
}

bool networkInitialized() {
    return gInitialized;
}

static typed(Vector*, Interface*) queryInterfaces() {
    struct ifaddrs* addrs;
    assert(!getifaddrs(&addrs));

    typed(Treemap*, Interface*) const interfacesMap = treemapCreate(nullptr);

    for (const struct ifaddrs* ifa = addrs; ifa; ifa = ifa->ifa_next) {
        if (!(ifa->ifa_flags & IFF_UP) ||
            !(ifa->ifa_flags & IFF_RUNNING) ||
            ifa->ifa_flags & IFF_LOOPBACK) continue;

        const struct sockaddr* const interfaceAddress = ifa->ifa_addr;

        if (interfaceAddress->sa_family == AF_PACKET) {
            Interface* const interface = malloc(sizeof *interface);
            strncpy(interface->name, ifa->ifa_name, sizeof interface->name);
            interface->index = if_nametoindex(interface->name);
            interface->mask = 0;

            treemapInsert(interfacesMap, hashValue(interface->name, sizeof interface->name), interface);
        } else if (interfaceAddress->sa_family == AF_INET6) {
            Interface* const interface = treemapSearchKey(interfacesMap, hashValue(ifa->ifa_name, sizeof interface->name));

            const struct in6_addr addr = ((struct sockaddr_in6*) (void*) ifa->ifa_addr)->sin6_addr;
            const struct in6_addr netmask = ((struct sockaddr_in6*) (void*) ifa->ifa_netmask)->sin6_addr;

            byte mask = 0;
            for (int j = 0; j < (int) sizeof netmask.s6_addr; mask += popcount(netmask.s6_addr[j]), j++) {}

            // not a link-local address
            if ((addr.s6_addr32[0] & bswap(0xffc00000)) != bswap(0xfe800000)) {
                // not a global routable address (DHCPv6 issued, hence - no mask/subnetPrefix - defaults to mask_max and no derivation from MAC address)
                if (mask < 128) {
                    interface->slaacAddress.native = addr;
                    interface->mask = mask;
                }
            } else {
                interface->linkLocalAddress.native = addr;
                assert(interface->mask == mask);
            }
        }
    }
    freeifaddrs(addrs);

    typed(Vector*, Interface*) const interfacesVector = vectorCreate(free);

    TreemapIterator* iterator;
    treemapIterateBegin(interfacesMap, iterator);
    Interface* interface;
    while ((interface = treemapIterate(iterator)))
        vectorAddBack(interfacesVector, interface);
    treemapIterateEnd(iterator);

    treemapDestroy(interfacesMap);
    return interfacesVector;
}

static void chooseInterface(const Interface* const address) {
    gChosenAddress.linkLocal = address->linkLocalAddress.vectorized;
    gChosenAddress.interfaceIndex = address->index;
    printMemory(&gChosenAddress.linkLocal, sizeof(vec16aw), PRINT_MEMORY_MODE_IPV6, false);
    printf("%%%u\n", gChosenAddress.interfaceIndex);
}

static void queryInterfaces0() {
    Vector* const interfaces = queryInterfaces();
    for (int i = 0; i < vectorSize(interfaces); i++) {
        const Interface* const interface = vectorGet(interfaces, i);
        printf("%s: ", interface->name);
        printMemory(&interface->slaacAddress, 16, PRINT_MEMORY_MODE_IPV6, false);
        printf("/%d", interface->mask);
        printf(" = ");
        printMemory(&interface->linkLocalAddress, 16, PRINT_MEMORY_MODE_IPV6, false);
        printf("%%%u\n", interface->index);
    }

    chooseInterface(vectorGet(interfaces, 0));

    vectorDestroy(interfaces);
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
