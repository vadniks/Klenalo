// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

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
    vec16uw address;
    // cryptographic signature
} PingPacket;

static atomic bool gInitialized = false;
static NET_Address gMulticastAddress = {};
static NET_DatagramSocket* gListenerSocket = nullptr; // listens for others
static NET_DatagramSocket* gInquirerSocket = nullptr; // pings others

void discoveryInit(const Address* const host) {
    assert(!gInitialized && networkInitialized());
    gInitialized = true;

    const auto hostAddress = (NET_Address) {
        nullptr,
        nullptr,
        nullptr,
        {1}, // so sdlNet won't try to free this
        NET_SUCCESS,
        &(struct addrinfo) {
            AI_NUMERICHOST,
            AF_INET6,
            SOCK_RAW,
            IPPROTO_IP,
            sizeof(struct sockaddr_in6),
            (struct sockaddr*) &(struct sockaddr_in6) {
                AF_INET6,
                0,
                0,
                host->linkLocal.native,
                host->interfaceIndex
            },
            nullptr,
            nullptr
        },
        nullptr
    };

    gMulticastAddress = (NET_Address) {
        nullptr,
        nullptr,
        nullptr,
        {1}, // so sdlNet won't try to free this
        NET_SUCCESS,
        &(struct addrinfo) {
            AI_NUMERICHOST,
            AF_INET6,
            SOCK_RAW,
            IPPROTO_IP,
            sizeof(struct sockaddr_in6),
            (struct sockaddr*) &(struct sockaddr_in6) {
                AF_INET6,
                0,
                0,
                {{.__u6_addr32 = {bswap(0x0000ff02), 0, 0, bswap(1)}}}, // ff02::1
                host->interfaceIndex
            },
            nullptr,
            nullptr
        },
        nullptr
    };

    const SDL_PropertiesID props = SDL_CreateProperties();
    assert(props);
    assert(SDL_SetBooleanProperty(props, NET_PROP_DATAGRAM_SOCKET_ALLOW_BROADCAST_BOOLEAN, true));

    leaky(assert(gListenerSocket = NET_CreateDatagramSocket((void*) &hostAddress, 8080, props));)
    SDL_DestroyProperties(props);

    leaky(assert(gInquirerSocket = NET_CreateDatagramSocket(&gMulticastAddress, 0, props));)
}

static void xlisten() {
    NET_Datagram* datagram = nullptr;
    do {
        assert(NET_ReceiveDatagram(gListenerSocket, &datagram));
        if (datagram && !networkIsLoopback((In6Addr*) &asSockaddrIn6(datagram->addr->ainfo->ai_addr)->sin6_addr))
            printf("received: %s %hu %d\n", datagram->addr->human_readable, datagram->port, datagram->buflen);
        NET_DestroyDatagram(datagram);
    } while (datagram);
    puts("listen");
}

void discoveryListen() {
    static ulong lastListen = 0;
    runPeriodically(&lastListen, LISTEN_PERIOD, xlisten);
}

static void ping() {
    PingPacket packet = {123, 0, {}};
    NET_SendDatagram(gInquirerSocket, &gMulticastAddress, 8080, &packet, sizeof packet);
    puts("ping");
}

void discoveryPing() {
    static ulong lastPing = 0;
    runPeriodically(&lastPing, PING_PERIOD, ping);
}

void discoveryQuit() {
    assert(gInitialized);
    gInitialized = false;

    NET_DestroyDatagramSocket(gListenerSocket);
    NET_DestroyDatagramSocket(gInquirerSocket);
}
