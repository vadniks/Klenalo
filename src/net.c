
#include <ifaddrs.h>
#include <endian.h>
#include <stdio.h>
#include "defs.h"
#include "net.h"

static atomic bool gInitialized = false;

void netInit(void) {
    assert(!gInitialized);
    gInitialized = true;

    struct ifaddrs* ifaddr;
    assert(!getifaddrs(&ifaddr));

    for (; ifaddr; ifaddr = ifaddr->ifa_next) {
        if (ifaddr->ifa_addr->sa_family != AF_INET) continue;

        const unsigned hostAddress = *(unsigned*) (ifaddr->ifa_addr->sa_data + 2);
        const unsigned subnetMask = *(unsigned*) (ifaddr->ifa_netmask->sa_data + 2);
        const unsigned netAddress = hostAddress & subnetMask;
        const unsigned broadcastAddress = netAddress + ~subnetMask;
        const unsigned hostMinAddress = netAddress + 0x01000000u; // only for little endian
        const unsigned hostMaxAddress = broadcastAddress - 0x01000000u;
        const unsigned hostsCount = htobe32(hostMaxAddress - hostMinAddress) + 1;

        unsigned mask = 0;
        for (unsigned n = subnetMask; n; n /= 2, mask++);

        if (hostAddress == 0x0100007f) continue; // loopback

        printf("%s %s\n", ifaddr->ifa_name, ifaddr->ifa_addr->sa_family == AF_INET ? "ipv4" : "ipv6");

#       define dotNotationLE(x) x & 0xff, (x >> 8) & 0xff, (x >> 16) & 0xff, (x >> 24) & 0xff
        printf("\tnetwork %u.%u.%u.%u/%u\n", dotNotationLE(netAddress), mask);
        printf("\tsubnet mask %u.%u.%u.%u\n", dotNotationLE(subnetMask));
        printf("\tbroadcast %u.%u.%u.%u\n", dotNotationLE(broadcastAddress));
        printf("\thost %u.%u.%u.%u\n", dotNotationLE(hostAddress));
        printf("\thost min %u.%u.%u.%u\n", dotNotationLE(hostMinAddress));
        printf("\thost max %u.%u.%u.%u\n", dotNotationLE(hostMaxAddress));
        printf("\thosts count %d\n", hostsCount);
#       undef dotNotationLE
    }

    freeifaddrs(ifaddr);
}

bool netInitialized(void) {
    return gInitialized;
}

static void ping(void) {


}

void netListen(void) {

}

void netQuit(void) {
    assert(gInitialized);
    gInitialized = false;
}