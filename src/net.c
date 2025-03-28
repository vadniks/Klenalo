
#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>
#include <sys/socket.h>
#include <netdb.h>
#include "lifecycle.h"
#include "consts.h"
#include "hashtable.h"
#include "net.h"

enum _NetMessageFlag : byte {
    NET_MESSAGE_FLAG_BROADCAST_HOST_DISCOVERY
};

typedef struct packed {
    union {
        const NetMessagePayload;
        struct {
            const char greeting[12];
            const byte version;
            const byte masterSessionSealPublicKey[CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE];
        };
    };
} HostDiscoveryBroadcastPayload;

typedef struct packed {
    union {
        const NetMessagePayload;
        const byte masterSessionSealPublicKey[CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE];
    };
} ConnectionHelloPayload;

typedef struct {
    const int address;
    const byte masterSessionSealPublicKey[CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE];
    SDLNet_StreamSocket* const socket;
} Connection;

// everything time-related is in milliseconds
static const short SUBNET_BROADCAST_SOCKET_PORT = 8080, SUBNET_CONNECTIONS_LISTENER_SERVER_PORT = 8081;
static const int MESSAGE_RECEIVE_TIME_WINDOW = 100;
static const int FETCH_SUBNETS_HOST_ADDRESSES_PERIOD = 100, SUBNET_BROADCAST_SEND_PERIOD = 1000, SUBNET_BROADCAST_RECEIVE_PERIOD = 250, ACCEPT_SUBNET_CONNECTIONS_PERIOD = 100;
static const int UDP_PACKET_MAX_SIZE = 512, BROADCAST_PAYLOAD_SIZE = UDP_PACKET_MAX_SIZE - (int) sizeof(NetMessage);
static const int HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE = sizeof(HostDiscoveryBroadcastPayload);
#define GREETING constsConcatenateTitleWith(" ping")

static atomic bool gInitialized = false;
static SDL_Mutex* gMutex = nullptr;

static List* gSubnetsHostsAddressesList = nullptr; // <int>
static atomic int gSelectedSubnetHostAddress = 0;

static SDLNet_DatagramSocket* gSubnetBroadcastSocket = nullptr;
static SDLNet_Server* gSubnetConnectionsListenerServer = nullptr; // it's a socket actually
static Hashtable* gConnectionsHashtable = nullptr; // <int - address, Connection*>

// TODO: non-blocking IO (select(), poll()) for connections but not the broadcast

void netInit(void) {
    assert(lifecycleInitialized() && !gInitialized);
    gInitialized = true;

    assert(SDLNet_Init());

    assert(gMutex = SDL_CreateMutex());

    gSubnetsHostsAddressesList = listCreate(false, nullptr);
}

bool netInitialized(void) {
    return gInitialized;
}

static void fetchSubnetsHostsAddresses(void) {
    SDL_LockMutex(gMutex);
    listClear(gSubnetsHostsAddressesList);

    int numberOfAddresses = 0;
    SDLNet_Address** const addresses = SDLNet_GetLocalAddresses(&numberOfAddresses);
    assert(numberOfAddresses && addresses);

    for (int i = 0; i < numberOfAddresses; i++) {
        const struct addrinfo* const info = *(struct addrinfo**) ((void*) addresses[i] + 32);
        const int addr = (int) swapBytes(*(unsigned*) (info->ai_addr->sa_data + 2));

        if (info->ai_addr->sa_family != AF_INET) continue;
        if (addr == INADDR_LOOPBACK) continue;

        listAddBack(gSubnetsHostsAddressesList, (void*) (long) addr);
    }

    SDLNet_FreeLocalAddresses(addresses);
    SDL_UnlockMutex(gMutex);
}

List* nullable netSubnetsHostsAddresses(void) {
    SDL_LockMutex(gMutex);
    List* const new = listCopy(gSubnetsHostsAddressesList, false, nullptr);
    SDL_UnlockMutex(gMutex);
    return new;
}

void netAddressToString(char* const buffer, const int address) {
    assert(lifecycleInitialized() && gInitialized);
    xmemset(buffer, 0, NET_ADDRESS_STRING_SIZE);
    const byte count = SDL_snprintf(
        buffer,
        NET_ADDRESS_STRING_SIZE,
        "%u.%u.%u.%u",
        (address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff
    );
    assert(count > 0 && count < NET_ADDRESS_STRING_SIZE);
}

static SDLNet_Address* resolveAddress(const int address) {
    char host[NET_ADDRESS_STRING_SIZE];
    netAddressToString(host, address);

    SDLNet_Address* const resolvedAddress = SDLNet_ResolveHostname(host);
    assert(resolvedAddress);
    assert(SDLNet_WaitUntilResolved(resolvedAddress, -1) == 1);
    return resolvedAddress;
}

static void generateHostDiscoveryBroadcastPayload(HostDiscoveryBroadcastPayload* const payload) {
    strncpy((char*) payload->greeting, GREETING, sizeof payload->greeting);
    unconst(payload->version) = 1;
    xmemcpy((byte*) payload->masterSessionSealPublicKey, cryptoMasterSessionSealPublicKey(), CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE);
}

static void destroyConnection(void* const connection) {
    SDLNet_DestroyStreamSocket(((Connection*) connection)->socket);
    xfree(connection);
}

void netStartBroadcastingAndListeningSubnet(const int subnetHostAddress) {
    assert(lifecycleInitialized() && gInitialized);
    assert(subnetHostAddress);

    SDL_LockMutex(gMutex);
    assert(!gSelectedSubnetHostAddress && !gSubnetBroadcastSocket && !gSubnetConnectionsListenerServer && !gConnectionsHashtable);

    gSelectedSubnetHostAddress = subnetHostAddress;

    SDLNet_Address* const address = resolveAddress(gSelectedSubnetHostAddress);
    assert(gSubnetBroadcastSocket = SDLNet_CreateDatagramSocket(address, SUBNET_BROADCAST_SOCKET_PORT));
    assert(gSubnetConnectionsListenerServer = SDLNet_CreateServer(address, SUBNET_CONNECTIONS_LISTENER_SERVER_PORT));
    SDLNet_UnrefAddress(address);

    assert(!setsockopt(
        *(int*) ((void*) gSubnetBroadcastSocket + 20),
        SOL_SOCKET,
        SO_BROADCAST,
        (int[1]) {1},
        sizeof(int)
    ));

    gConnectionsHashtable = hashtableCreate(false, destroyConnection);

    SDL_UnlockMutex(gMutex);
}

void netStopBroadcastingAndListeningSubnet(void) {
    assert(lifecycleInitialized() && gInitialized);

    SDL_LockMutex(gMutex);
    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket && gSubnetConnectionsListenerServer && gConnectionsHashtable);

    gSelectedSubnetHostAddress = 0;

    SDLNet_DestroyDatagramSocket(gSubnetBroadcastSocket);
    gSubnetBroadcastSocket = nullptr;

    SDLNet_DestroyServer(gSubnetConnectionsListenerServer);
    gSubnetConnectionsListenerServer = nullptr;

    hashtableDestroy(gConnectionsHashtable);
    gConnectionsHashtable = nullptr;

    SDL_UnlockMutex(gMutex);
}

static void broadcastSubnetForHosts(void) {
    const int messageSize = NET_MESSAGE_SIZE + HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE;
    staticAssert(messageSize <= UDP_PACKET_MAX_SIZE);

    NetMessage* const message = xalloca(messageSize);
    unconst(message->flag) = NET_MESSAGE_FLAG_BROADCAST_HOST_DISCOVERY;
    unconst(message->timestamp) = lifecycleCurrentTimeMillis();
    unconst(message->index) = 0;
    unconst(message->count) = 1;
    unconst(message->from) = gSelectedSubnetHostAddress;
    unconst(message->to) = NET_MESSAGE_TO_EVERYONE;
    unconst(message->size) = HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE;
    generateHostDiscoveryBroadcastPayload((HostDiscoveryBroadcastPayload*) message->payload);
    cryptoMasterSign(
        (byte*) message,
        messageSize - CRYPTO_SIGNATURE_SIZE,
        (byte*) message->signature
    );

    SDLNet_Address* const address = resolveAddress(message->to);

    SDL_LockMutex(gMutex);

    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket);
    assert(message->from == gSelectedSubnetHostAddress);
    assert(SDLNet_SendDatagram(gSubnetBroadcastSocket, address, SUBNET_BROADCAST_SOCKET_PORT, message, messageSize));

    SDL_UnlockMutex(gMutex);

    SDLNet_UnrefAddress(address);
}

static void listenSubnetForBroadcasts(void) {
    SDLNet_Datagram* datagram;

    SDL_LockMutex(gMutex);
    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket);
    assert(SDLNet_ReceiveDatagram(gSubnetBroadcastSocket, &datagram));
    SDL_UnlockMutex(gMutex);

    if (datagram) {
        debugArgs("%s:%u %d", SDLNet_GetAddressString(datagram->addr), datagram->port, datagram->buflen)
        printMemory(datagram->buf, datagram->buflen, PRINT_MEMORY_MODE_TRY_STR_HEX_FALLBACK);

        if (datagram->buflen == NET_MESSAGE_SIZE + HOST_DISCOVERY_BROADCAST_PAYLOAD_SIZE) // as anyone can send anything anywhere over udp
            assert(cryptoCheckMasterSigned(datagram->buf, datagram->buflen - CRYPTO_SIGNATURE_SIZE, ((NetMessage*) datagram->buf)->signature)); // TODO: extract tot a separate function

        // TODO: try to connect to that host if haven't already

        SDLNet_DestroyDatagram(datagram);
    }
}

static void acceptConnections(void) {
    SDLNet_StreamSocket* connectionSocket;
    while (true) {
        SDL_LockMutex(gMutex);
        assert(SDLNet_AcceptClient(gSubnetConnectionsListenerServer, &connectionSocket));
        SDL_UnlockMutex(gMutex);

        if (!connectionSocket) return;

        SDLNet_Address* const addr = SDLNet_GetStreamSocketAddress(connectionSocket);
        assert(addr);
        const struct addrinfo* const info = *(struct addrinfo**) ((void*) addr + 32); // TODO: extract tot a separate function
        const int address = (int) swapBytes(*(unsigned*) (info->ai_addr->sa_data + 2));
        SDLNet_UnrefAddress(addr);

        // TODO: allocate iterators on the callers' stacks instead of the heap

        SDL_LockMutex(gMutex);
        Connection* const connection = hashtableGet(gConnectionsHashtable, hashtableHashPrimitive(address));
        SDL_UnlockMutex(gMutex);
        if (connection) {
            assert(connection->address == address);
            SDLNet_DestroyStreamSocket(connectionSocket); // connection is already established
            continue;
        }

        const unsigned long timestamp = lifecycleCurrentTimeMillis();
        const int messageSize = NET_MESSAGE_SIZE + sizeof(ConnectionHelloPayload);
        NetMessage* const message = xalloca(messageSize);
        int bytesLeft; // TODO: extract tot a separate function
        SDL_LockMutex(gMutex);
        for (bytesLeft = messageSize; bytesLeft >= 0;) {
            const int read = SDLNet_ReadFromStreamSocket(connectionSocket, (void*) message + (messageSize - bytesLeft), bytesLeft);
            if (read < 1 || lifecycleCurrentTimeMillis() - timestamp >= MESSAGE_RECEIVE_TIME_WINDOW) break;
            bytesLeft -= read;
        }
        SDL_UnlockMutex(gMutex);

        if (bytesLeft) {
            SDLNet_DestroyStreamSocket(connectionSocket);
            continue;
        }

        if (message->from != address) {
            SDLNet_DestroyStreamSocket(connectionSocket);
            continue;
        }

        const ConnectionHelloPayload* const payload = (void*) message->payload;

        if (!cryptoCheckMasterSigned((byte*) message, messageSize - CRYPTO_SIGNATURE_SIZE, message->signature)) { // TODO: extract tot a separate function
            SDLNet_DestroyStreamSocket(connectionSocket);
            continue;
        }

        Connection* const newConnection = xmalloc(sizeof *newConnection);
        unconst(newConnection->address) = address;
        xmemcpy((void*) newConnection->masterSessionSealPublicKey, payload->masterSessionSealPublicKey, CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE);
        unconst(newConnection->socket) = connectionSocket;

        SDL_LockMutex(gMutex);
        hashtablePut(gConnectionsHashtable, hashtableHashPrimitive(address), newConnection);
        SDL_UnlockMutex(gMutex);
    }
}

static void runPeriodically(const unsigned long currentMillis, unsigned long* const lastRunMillis, const int period, void (* const action)(void)) {
    if (currentMillis - *lastRunMillis < (unsigned) period) return;
    *lastRunMillis = currentMillis;
    action();
}

void netLoop(void) {
    assert(lifecycleInitialized() && gInitialized);

    const unsigned long currentMillis = lifecycleCurrentTimeMillis();
    static unsigned long lastSubnetsHostsAddressesFetch = 0, lastBroadcastSend = 0, lastBroadcastReceive = 0, lastClientsAccept = 0;

    runPeriodically(currentMillis, &lastSubnetsHostsAddressesFetch, FETCH_SUBNETS_HOST_ADDRESSES_PERIOD, fetchSubnetsHostsAddresses);

    if (gSelectedSubnetHostAddress) {
        runPeriodically(currentMillis, &lastBroadcastSend, SUBNET_BROADCAST_SEND_PERIOD, broadcastSubnetForHosts);
        runPeriodically(currentMillis, &lastBroadcastReceive, SUBNET_BROADCAST_RECEIVE_PERIOD, listenSubnetForBroadcasts);
        runPeriodically(currentMillis, &lastClientsAccept, ACCEPT_SUBNET_CONNECTIONS_PERIOD, acceptConnections);
    }
}

void netQuit(void) {
    assert(gInitialized);

    if (gSelectedSubnetHostAddress) netStopBroadcastingAndListeningSubnet();

    gInitialized = false;

    listDestroy(gSubnetsHostsAddressesList);

    SDL_DestroyMutex(gMutex);

    SDLNet_Quit();
}
