// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#include "../../common/crypto/crypto.h"
#include "../../common/crypto/credentials.h"
#include "../../common/collections/treemap.h"
#include "../../common/collections/vector.h"
#include "network.h"
#include "discovery.h"

enum {
    LISTENING_PORT = 8080,
    BUFFER_SIZE = 128,
    PING_PACKET_MAGIC = 0,
    PONG_PACKET_MAGIC = 1
};

typedef struct xpacked {
    long magic;
    byte version;
    CryptoKey publicKey;
} PingPongPacketPayload;

typedef struct xpacked {
    PingPongPacketPayload;
    CryptoSignature signature;
} PingPongPacket;

static constexpr struct in6_addr MULTICAST_ADDR = {{.__u6_addr32 = {bswap(0xff020000), 0, 0, bswap(1)}}};

static atomic bool gInitialized = false;
static int gListenerSocket = 0; // listens for others
static int gInquirerSocket = 0; // pings
static int gInterfaceIndex = 0;

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

static void createListener() {
    assert((gListenerSocket = socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) >= 0);
    assert(!setsockopt(gListenerSocket, SOL_SOCKET, SO_REUSEADDR, anonVar(1), sizeof(int)));

    const struct sockaddr_in6 addr = {
        AF_INET6,
        bswap((ushort) LISTENING_PORT),
        0,
        in6addr_any,
        gInterfaceIndex
    };
    assert(!bind(gListenerSocket, (const struct sockaddr*) &addr, sizeof addr));

    const struct ipv6_mreq request = {MULTICAST_ADDR, gInterfaceIndex};
    assert(!setsockopt(gListenerSocket, IPPROTO_IPV6, IPV6_JOIN_GROUP, &request, sizeof request));
}

static void createInquirer() {
    assert((gInquirerSocket = socket(AF_INET6, SOCK_DGRAM | SOCK_NONBLOCK, IPPROTO_UDP)) >= 0);
    assert(!setsockopt(gInquirerSocket, IPPROTO_IPV6, IPV6_MULTICAST_IF, &gInterfaceIndex, sizeof(unsigned)));

    const struct sockaddr_in6 addr = {
        AF_INET6,
        0,
        0,
        MULTICAST_ADDR,
        gInterfaceIndex
    };
    assert(!bind(gInquirerSocket, (const struct sockaddr*) &addr, sizeof addr));
}

// TODO: server actually doesn't need SDL library at all

void discoveryInit() {
    assert(!gInitialized && networkInitialized());
    gInitialized = true;

    Vector* const interfaces = queryInterfaces();
    const Interface* const interface = vectorGet(interfaces, 0);
    const Address host = {interface->linkLocalAddress, interface->index};
    vectorDestroy(interfaces);

    gInterfaceIndex = host.interfaceIndex;

    createListener();
    createInquirer();
}

static bool isLoopback(const In6Addr* const addr) {
    const vec16uq loopback = {0, bswap(1l)};
    return compareXmmVectors((vec16uq) addr->vectorized, loopback);
}

void discoveryListen() {
    byte buffer[BUFFER_SIZE] = {};
    int readSize = 0;
     while (true) {
         struct sockaddr_in6 addr;
         unsigned socklen = 0;
         readSize = (int) recvfrom(gListenerSocket, buffer, sizeof buffer, 0, (struct sockaddr*) &addr, &socklen);

printf("%d\n", readSize); //

         if (readSize < 0 && errno == EWOULDBLOCK) break; // EWOULDBLOCK = EAGAIN
         else if (readSize < 0) assert(false);
         else if (readSize == 0 || addr.sin6_family != AF_INET6) continue;

         if (readSize == sizeof(PingPongPacket) /*&& !isOurPacket(datagram)*/) {
             printf("received: ");
             printMemory(&addr.sin6_addr, 16, PRINT_MEMORY_MODE_IPV6, false);
             printf(" %hu %u\n", addr.sin6_port, addr.sin6_scope_id);

             const auto* const packet = (PingPongPacket*) buffer;
             if (packet->magic == PING_PACKET_MAGIC) {
                 CryptoKey globalPublicKey, globalSecretKey;
                 credentialsGlobalSignKey(&globalPublicKey, true);
                 credentialsGlobalSignKey(&globalSecretKey, false);

                 const bool verified = cryptoSignVerify(&packet->signature, packet, sizeof(PingPongPacketPayload), &globalPublicKey);
                 assert(verified);

                 // if (verified) {
                 //     PingPongPacket packet = {{PONG_PACKET_MAGIC, PROGRAM_VERSION, {}}, {}};
                 //     cryptoSign(&packet.signature, &packet, sizeof(PingPongPacketPayload), &globalPublicKey, &globalSecretKey);
                 //     assert(NET_SendDatagram(gInquirerSocket, datagram->addr, LISTENING_PORT, &packet, sizeof packet));
                 //     puts("pong");
                 // }

                 cryptoZeroMemory(&globalPublicKey, sizeof globalPublicKey);
                 cryptoZeroMemory(&globalSecretKey, sizeof globalSecretKey);
             }
         }
     };
     puts("listen");
}

void discoveryPing() {
    CryptoKey globalPublicKey, globalSecretKey;
    credentialsGlobalSignKey(&globalPublicKey, true);
    credentialsGlobalSignKey(&globalSecretKey, false);

    PingPongPacket packet = {{PING_PACKET_MAGIC, PROGRAM_VERSION, {}}, {}};
    cryptoSign(&packet.signature, &packet, sizeof(PingPongPacketPayload), &globalPublicKey, &globalSecretKey);

    cryptoZeroMemory(&globalPublicKey, sizeof globalPublicKey);
    cryptoZeroMemory(&globalSecretKey, sizeof globalSecretKey);

    //

    assert(sendto(
        gInquirerSocket,
        &packet,
        sizeof packet,
        0,
        (struct sockaddr*) &(struct sockaddr_in6) {AF_INET6, bswap((ushort) LISTENING_PORT), 0, MULTICAST_ADDR, gInterfaceIndex},
        sizeof(struct sockaddr_in6)
    ) == sizeof packet);

    puts("ping");
}

void discoveryQuit() {
    assert(gInitialized);
    gInitialized = false;

    close(gListenerSocket);
    close(gInquirerSocket);
}
