// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#include "../../common/crypto/crypto.h"
#include "../../common/crypto/credentials.h"
#include "network.h"
#include "discovery.h"

enum {
    PING_PORT = 8080,
    LISTEN_PERIOD = 500,
    PING_PERIOD = 1000
};

typedef struct xpacked {
    long magic;
    byte version;
    // vec16uw address; // unnecessary
    // public key for p2p connection
} PingPacketPayload;

typedef struct xpacked {
    PingPacketPayload;
    CryptoSignature signature;
} PingPacket;

static atomic bool gInitialized = false;
static NET_Address gMulticastAddress = {};
static NET_DatagramSocket* gListenerSocket = nullptr; // listens for others
static NET_DatagramSocket* gInquirerSocket = nullptr; // pings others

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
    };

void discoveryInit(const Address* const host) {
    assert(!gInitialized && networkInitialized());
    gInitialized = true;

    const auto hostAddress = makeNetAddress(host->linkLocal.native, host->interfaceIndex, PING_PORT);
    gMulticastAddress = makeNetAddress(
        ((struct in6_addr) {{.__u6_addr32 = {bswap(0x0000ff02), 0, 0, bswap(1)}}}),
        host->interfaceIndex,
        0
    )

    const SDL_PropertiesID props = SDL_CreateProperties();
    assert(props);
    assert(SDL_SetBooleanProperty(props, NET_PROP_DATAGRAM_SOCKET_ALLOW_BROADCAST_BOOLEAN, true));

    leaky(assert(gListenerSocket = NET_CreateDatagramSocket((void*) &hostAddress, PING_PORT, props));)
    SDL_DestroyProperties(props);

    leaky(assert(gInquirerSocket = NET_CreateDatagramSocket(&gMulticastAddress, 0, props));)
}

static bool isOurPacket(const NET_Datagram* const datagram) {
    return networkIsLoopback((In6Addr*) &asSockaddrIn6(datagram->addr->ainfo->ai_addr)->sin6_addr);
}

static void xlisten() {
    NET_Datagram* datagram = nullptr;
    do {
        assert(NET_ReceiveDatagram(gListenerSocket, &datagram));

        if (datagram && datagram->buflen == sizeof(PingPacket) /*&& !isOurPacket(datagram)*/) {
            printf("received: %s %hu %d\n", datagram->addr->human_readable, datagram->port, datagram->buflen);

            CryptoKey key;
            credentialsGlobalSignKey(&key, true);

            const auto* const packet = (PingPacket*) datagram->buf;
            assert(cryptoSignVerify(&packet->signature, packet, sizeof(PingPacketPayload), &key));

            cryptoZeroMemory(&key, sizeof key);
        }

        NET_DestroyDatagram(datagram);
    } while (datagram);
    puts("listen");
}

void discoveryListen() {
    static ulong last = 0;
    runPeriodically(&last, LISTEN_PERIOD, xlisten);
}

static void ping() {
    CryptoKey publicKey, secretKey;
    credentialsGlobalSignKey(&publicKey, true);
    credentialsGlobalSignKey(&secretKey, false);

    PingPacket packet = {{123, 0}, {}};
    cryptoSign(&packet.signature, &packet, sizeof(PingPacketPayload), &publicKey, &secretKey);

    cryptoZeroMemory(&publicKey, sizeof publicKey);
    cryptoZeroMemory(&secretKey, sizeof secretKey);

    assert(NET_SendDatagram(gInquirerSocket, &gMulticastAddress, PING_PORT, &packet, sizeof packet));
    puts("ping");
}

void discoveryPing() {
    static ulong last = 0;
    runPeriodically(&last, PING_PERIOD, ping);
}

void discoveryQuit() {
    assert(gInitialized);
    gInitialized = false;

    NET_DestroyDatagramSocket(gListenerSocket);
    NET_DestroyDatagramSocket(gInquirerSocket);
}
