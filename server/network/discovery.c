// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#include "../../common/crypto/crypto.h"
#include "../../common/crypto/credentials.h"
#include "../../common/collections/treemap.h"
#include "../../common/collections/vector.h"
#include "../../common/collections/deque.h"
#include "shared.h"
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

typedef struct {
    PingPongPacket packet;
    struct in6_addr address;
} DeferredPongPacket;

typedef enum {
    DISCOVERED_STATE_PING_RECEIVED,
    DISCOVERED_STATE_VERIFIED,
    DISCOVERED_STATE_PONG_RECEIVED
} DiscoveredState;

typedef struct {

} DiscoveredNode;

static constexpr struct in6_addr MULTICAST_ADDR = {{.__u6_addr32 = {bswap(0xff020000), 0, 0, bswap(1)}}};

static bool gInitialized = false;
static int gListenerSocket = 0; // listens for others
static int gInquirerSocket = 0; // pings
static struct in6_addr gHostAddress = {};
static unsigned gInterfaceIndex = 0;
static CryptoKey gListenerPublicKey, glistenerSecretKey, gInquirerPublicKey, gInquirerSecretKey; // TODO: temp
static typed(Deque*, *) gDeferredPongPacketsQueue = nullptr;
// static typed(Treemap*, int, )

static typed(Vector*, Interface*) queryInterfaces();
static void createListener();
static void createInquirer();

void discoveryInit() {
    assert(!gInitialized && networkInitialized());
    gInitialized = true;

    Vector* const interfaces = queryInterfaces();
    const Interface* const interface = vectorGet(interfaces, 0);
    const Address host = {interface->linkLocalAddress, interface->index};
    vectorDestroy(interfaces);

    gHostAddress = host.linkLocal.native;
    gInterfaceIndex = host.interfaceIndex;

    createListener();
    createInquirer();

    cryptoKeypair(&gListenerPublicKey, &glistenerSecretKey);
    cryptoKeypair(&gInquirerPublicKey, &gInquirerSecretKey);

    gDeferredPongPacketsQueue = dequeCreate(free);
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

static bool isLoopback(const struct in6_addr* const addr) {
    const struct in6_addr loopback = {.__in6_u.__u6_addr32 = {0, 0, 0, bswap(1)}};
    return !memcmp(addr, &loopback, sizeof loopback);
}

static bool isThisHost(const struct in6_addr* const addr) {
    return !memcmp(&gHostAddress, addr, sizeof gHostAddress);
}

static void processIncomingPing(const struct sockaddr_in6* const addr, const PingPongPacket* const pingPacket) {
    printf("process ping %hu ", addr->sin6_port); printMemory(&addr->sin6_addr, sizeof(struct in6_addr), PRINT_MEMORY_MODE_IPV6, true);
    assert(addr->sin6_scope_id == gInterfaceIndex);

    CryptoKey globalPublicKey, globalSecretKey;
    credentialsGlobalSignKey(&globalPublicKey, true);
    credentialsGlobalSignKey(&globalSecretKey, false);

    const bool verified = cryptoSignVerify(&pingPacket->signature, pingPacket, sizeof(PingPongPacketPayload), &globalPublicKey);
    assert(verified);

    if (verified) {
        DeferredPongPacket* const deferred = malloc(sizeof *deferred);
        deferred->packet = (PingPongPacket) {{PONG_PACKET_MAGIC, PROGRAM_VERSION, gListenerPublicKey}, {}};
        cryptoSign(&deferred->packet.signature, &deferred->packet, sizeof(PingPongPacketPayload), &globalPublicKey, &globalSecretKey);
        deferred->address = addr->sin6_addr;
        dequeQueuePush(gDeferredPongPacketsQueue, deferred);
    } // else delete & ignore the node

    cryptoZeroMemory(&globalPublicKey, sizeof globalPublicKey);
    cryptoZeroMemory(&globalSecretKey, sizeof globalSecretKey);
}

static void processIncomingPong(const struct sockaddr_in6* const addr, const PingPongPacket* const packet) {
    printf("process pong %hu ", addr->sin6_port); printMemory(&addr->sin6_addr, sizeof(struct in6_addr), PRINT_MEMORY_MODE_IPV6, false);

    CryptoKey key;
    const bool successful = cryptoExchangeKeys(&key, &packet->publicKey, &glistenerSecretKey); // if not - retry

    printf(" ");
    printMemory(&key, 32, PRINT_MEMORY_MODE_HEXDUMP, true);

    printf("ping-pong %s\n", boolToStr(successful));
}

void discoveryListen() {
    assert(gInitialized && networkInitialized());

    byte buffer[BUFFER_SIZE] = {}; // TODO: select/poll/epoll
    int readSize = 0;

     while (true) {
         struct sockaddr_in6 addr;
         memset(&addr, 0xff, sizeof addr);
         unsigned socklen = sizeof addr;
         readSize = (int) recvfrom(gListenerSocket, buffer, sizeof buffer, 0, (struct sockaddr*) &addr, &socklen);

         if (readSize < 0 && errno == EWOULDBLOCK) break; // EWOULDBLOCK = EAGAIN
         else if (readSize < 0) assert(false);
         else if (readSize == 0 || addr.sin6_family != AF_INET6) continue;

         if (readSize == sizeof(PingPongPacket) && !isLoopback(&addr.sin6_addr) && !isThisHost(&addr.sin6_addr)) {
             const auto* const packet = (PingPongPacket*) buffer;
             switch (packet->magic) {
                 case PING_PACKET_MAGIC: processIncomingPing(&addr, packet); break;
                 case PONG_PACKET_MAGIC: processIncomingPong(&addr, packet); break;
                 default: assert(false);
             }
         }
     }
     puts("listen");
}

void discoveryPing() {
    assert(gInitialized && networkInitialized());

    CryptoKey globalPublicKey, globalSecretKey;
    credentialsGlobalSignKey(&globalPublicKey, true);
    credentialsGlobalSignKey(&globalSecretKey, false);

    PingPongPacket packet = {{PING_PACKET_MAGIC, PROGRAM_VERSION, gInquirerPublicKey}, {}};
    cryptoSign(&packet.signature, &packet, sizeof(PingPongPacketPayload), &globalPublicKey, &globalSecretKey);

    cryptoZeroMemory(&globalPublicKey, sizeof globalPublicKey);
    cryptoZeroMemory(&globalSecretKey, sizeof globalSecretKey);

    //

    assert(sendto(
        gInquirerSocket,
        &packet,
        sizeof packet,
        0,
        (struct sockaddr*) &(struct sockaddr_in6) {
            AF_INET6,
            bswap((ushort) LISTENING_PORT),
            0,
            MULTICAST_ADDR,
            gInterfaceIndex
        },
        sizeof(struct sockaddr_in6)
    ) == sizeof packet);

    puts("ping");
}

void discoveryPong() {
    assert(gInitialized && networkInitialized());

    while (dequeQueuePeek(gDeferredPongPacketsQueue)) {
        DeferredPongPacket* const deferred = dequeQueuePop(gDeferredPongPacketsQueue, false);

        assert(sendto(
            gInquirerSocket,
            &deferred->packet,
            sizeof(PingPongPacket),
            0,
            (struct sockaddr*) &(struct sockaddr_in6) {
                AF_INET6,
                bswap((ushort) LISTENING_PORT),
                0,
                deferred->address,
                gInterfaceIndex
            },
            sizeof(struct sockaddr_in6)
        ) == sizeof(PingPongPacket));

        free(deferred);

        puts("pong");
    }
}

void discoveryQuit() {
    assert(gInitialized);
    gInitialized = false;

    dequeDestroy(gDeferredPongPacketsQueue);

    close(gListenerSocket);
    close(gInquirerSocket);
}
