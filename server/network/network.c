
#include <SDL3_net/SDL_net.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
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

static bool compareAddresses(const struct in6_addr* const addr1, const struct in6_addr* const addr2) {
    const auto xaddr1 = *(vec16uq*) addr1;
    const auto xaddr2 = *(vec16uq*) addr2;
    return compareXmmVectors(xaddr1, xaddr2);
}

static void queryInterfaces() {
    struct ifaddrs* addrs;
    assert(!getifaddrs(&addrs));

    Vector* const addresses = vectorCreate(free); // <Address*>

    puts("");
    for (const struct ifaddrs* interface = addrs; interface; interface = interface->ifa_next) {
        if ((interface->ifa_flags & IFF_UP) != IFF_UP ||
            (interface->ifa_flags & IFF_RUNNING) != IFF_RUNNING ||
            (interface->ifa_flags & IFF_LOOPBACK) == IFF_LOOPBACK) continue;

        // if (address->ainfo->ai_socktype != SOCK_DGRAM) continue; // remove tcp and unix
        // if (address->ainfo->ai_family != AF_INET6) continue; // remove ipv4 and unix
        // if (compareAddresses(&((struct sockaddr_in6*) (void*) address->ainfo->ai_addr)->sin6_addr, &in6addr_loopback)) continue; // remove loopback
        // remove public

        // const struct sockaddr_in6
        const struct sockaddr* const address = interface->ifa_addr;

        if (address->sa_family == AF_PACKET) {
            Address* const ourAddress = malloc(sizeof *ourAddress);
            strncpy(ourAddress->name, interface->ifa_name, sizeof ourAddress->name);
            ourAddress->mask = 0;
            vectorAddBack(addresses, ourAddress);
        } else if (address->sa_family == AF_INET6) {
            for (int i = 0; i < vectorSize(addresses); i++) {
                Address* const ourAddress = vectorGet(addresses, i);

                const struct in6_addr addr = ((struct sockaddr_in6*) (void*) interface->ifa_addr)->sin6_addr;
                const struct in6_addr netmask = ((struct sockaddr_in6*) (void*) interface->ifa_netmask)->sin6_addr;

                int mask = 0;
                for (int j = 0; j < 16; j++)
                    mask += __builtin_popcount(netmask.__in6_u.__u6_addr8[j]);

                if (addr.__in6_u.__u6_addr16[0] != __builtin_bswap16(0xfe80) && inRange(ourAddress->mask, mask, 0x7f) && !strncmp(interface->ifa_name, ourAddress->name, sizeof ourAddress->name)) {
                    ourAddress->address = addr;
                    ourAddress->mask = mask;
                }

                printf("%s ", interface->ifa_name);
                printMemory(&interface->ifa_flags, 4, PRINT_MEMORY_MODE_BINARY, false);
                printf(" ");
                printMemory(addr.__in6_u.__u6_addr8, 16, PRINT_MEMORY_MODE_HEXDUMP, false);
                printf(" ");
                printMemory(addr.__in6_u.__u6_addr8, 16, PRINT_MEMORY_MODE_HEXDUMP, false);
                printf("\n");
            }
        }
    }

    for (int i = 0; i < vectorSize(addresses); i++) {
        Address* const address = vectorGet(addresses, i);
        printf("%s ", address->name);

        printMemory(&address->address, 16, PRINT_MEMORY_MODE_IPV6, false);
        printf(" ");

        printf("%d\n", address->mask);
    }
    puts("");

    freeifaddrs(addrs);
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
    runPeriodically(currentTimeMillis(), &lastSubnetsQuery, 10000, queryInterfaces);
}

void networkQuit() {
    assert(gInitialized);
    gInitialized = false;

    NET_Quit();
}
