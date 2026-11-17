
#include "../../common/collections/treemap.h"
#include "../../common/collections/vector.h"
#include "../core/lifecycle.h"
#include "discovery.h"
#include "network.h"

static atomic bool gInitialized = false;
static Address gChosenHostAddress = {0};

static void queryInterfaces0();

void networkInit() {
    assert(!gInitialized && lifecycleInitialized());
    gInitialized = true;

    assert(NET_Init());

    queryInterfaces0();
    discoveryInit(&gChosenHostAddress);
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

            const struct in6_addr addr = asSockaddrIn6(ifa->ifa_addr)->sin6_addr;
            const struct in6_addr netmask = asSockaddrIn6(ifa->ifa_netmask)->sin6_addr;

            byte mask = 0;
            for (int j = 0; j < (int) sizeof netmask.s6_addr; mask += popcount(netmask.s6_addr[j]), j++) {}

            if ((addr.s6_addr32[0] & bswap(0xffc00000)) == bswap(0xfe800000)) {
                interface->linkLocalAddress.native = addr;
                interface->mask = mask;
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
    gChosenHostAddress.linkLocal = address->linkLocalAddress;
    gChosenHostAddress.interfaceIndex = address->index;
    printMemory(&gChosenHostAddress.linkLocal, sizeof(In6Addr), PRINT_MEMORY_MODE_IPV6, false);
    printf("%%%u\n", gChosenHostAddress.interfaceIndex);
}

static void queryInterfaces0() {
    Vector* const interfaces = queryInterfaces();
    chooseInterface(vectorGet(interfaces, 0));
    vectorDestroy(interfaces);
}

bool networkIsLoopback(const In6Addr* const addr) {
    const vec16uq loopback = {0, bswap(1l)};
    return compareXmmVectors((vec16uq) addr->vectorized, loopback);
}

void networkRunPeriodically(
    ulong* const lastRunMillis,
    const int periodMillis,
    void (* const action)(void)
) {
    const ulong currentMillis = currentTimeMillis();
    if (currentMillis - *lastRunMillis < (unsigned) periodMillis) return;
    *lastRunMillis = currentMillis;
    action();
}

void networkLoopBody() {
    discoveryListen();
    discoveryPing();
}

void networkQuit() {
    assert(gInitialized);
    gInitialized = false;

    discoveryQuit();
    NET_Quit();
}
