// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#include "../../common/crypto/crypto.h"
#include "../../common/crypto/credentials.h"
#include "../../common/collections/treemap.h"
#include "../../common/collections/vector.h"
#include "network.h"
#include "discovery.h"

enum {
    PING_PORT = 8080,
    PING_PACKET_MAGIC = 0
};

typedef struct xpacked {
    long magic;
    byte version;
    CryptoKey publicKey;
} PingPacketPayload;

typedef struct xpacked {
    PingPacketPayload;
    CryptoSignature signature;
} PingPacket;

static atomic bool gInitialized = false;
static NET_Address gMulticastAddress = {};
static NET_DatagramSocket* gListenerSocket = nullptr; // listens for others
static NET_DatagramSocket* gInquirerSocket = nullptr; // pings others

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

#define makeNetAddress(addr, interfaceIndex, port) \
    (NET_Address) { \
        nullptr, \
        nullptr, \
        nullptr, \
        {1}, \
        NET_SUCCESS, \
        &(struct addrinfo) { \
            AI_NUMERICHOST, \
            AF_INET6, \
            SOCK_RAW, \
            IPPROTO_IP, \
            sizeof(struct sockaddr_in6), \
            (struct sockaddr*) &(struct sockaddr_in6) { \
                AF_INET6, \
                port, \
                0, \
                addr, \
                interfaceIndex \
            }, \
            nullptr, \
            nullptr \
        }, \
        nullptr \
    }

void discoveryInit() {
    assert(!gInitialized && networkInitialized());
    gInitialized = true;

    Vector* const interfaces = queryInterfaces();
    const Interface* const interface = vectorGet(interfaces, 0);
    const Address host = {interface->linkLocalAddress, interface->index};
    vectorDestroy(interfaces);

    const auto hostAddress = makeNetAddress(host.linkLocal.native, host.interfaceIndex, PING_PORT);
    gMulticastAddress = makeNetAddress(
        ((struct in6_addr) {{.__u6_addr32 = {bswap(0x0000ff02), 0, 0, bswap(1)}}}),
        host.interfaceIndex,
        0
    );

    const SDL_PropertiesID props = SDL_CreateProperties();
    assert(props);
    assert(SDL_SetBooleanProperty(props, NET_PROP_DATAGRAM_SOCKET_ALLOW_BROADCAST_BOOLEAN, true));

    leaky(assert(gListenerSocket = NET_CreateDatagramSocket((void*) &hostAddress, PING_PORT, props));)
    SDL_DestroyProperties(props);

    leaky(assert(gInquirerSocket = NET_CreateDatagramSocket(&gMulticastAddress, 0, props));)
}

static bool isLoopback(const In6Addr* const addr) {
    const vec16uq loopback = {0, bswap(1l)};
    return compareXmmVectors((vec16uq) addr->vectorized, loopback);
}

static bool isOurPacket(const NET_Datagram* const datagram) {
    return isLoopback((In6Addr*) &asSockaddrIn6(datagram->addr->ainfo->ai_addr)->sin6_addr);
}

void discoveryListen() {
    NET_Datagram* datagram = nullptr;
    do {
        assert(NET_ReceiveDatagram(gListenerSocket, &datagram));

        if (datagram && datagram->buflen == sizeof(PingPacket) /*&& !isOurPacket(datagram)*/) {
            printf("received: %s %hu %d\n", datagram->addr->human_readable, datagram->port, datagram->buflen);

            CryptoKey globalPublicKey;
            credentialsGlobalSignKey(&globalPublicKey, true);

            const auto* const packet = (PingPacket*) datagram->buf;
            assert(cryptoSignVerify(&packet->signature, packet, sizeof(PingPacketPayload), &globalPublicKey));

            cryptoZeroMemory(&globalPublicKey, sizeof globalPublicKey);
        }

        NET_DestroyDatagram(datagram);
    } while (datagram);
    puts("listen");
}

void discoveryPing() {
    CryptoKey globalPublicKey, globalSecretKey;
    credentialsGlobalSignKey(&globalPublicKey, true);
    credentialsGlobalSignKey(&globalSecretKey, false);

    PingPacket packet = {{PING_PACKET_MAGIC, PROGRAM_VERSION, {}}, {}};
    cryptoSign(&packet.signature, &packet, sizeof(PingPacketPayload), &globalPublicKey, &globalSecretKey);

    cryptoZeroMemory(&globalPublicKey, sizeof globalPublicKey);
    cryptoZeroMemory(&globalSecretKey, sizeof globalSecretKey);

    assert(NET_SendDatagram(gInquirerSocket, &gMulticastAddress, PING_PORT, &packet, sizeof packet));
    puts("ping");
}

void discoveryQuit() {
    assert(gInitialized);
    gInitialized = false;

    NET_DestroyDatagramSocket(gListenerSocket);
    NET_DestroyDatagramSocket(gInquirerSocket);
}
