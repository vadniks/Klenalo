//
//#include <SDL3/SDL.h>
//#include <SDL3_net/SDL_net.h>
//#include <sys/socket.h>
//#include <netdb.h>
//#include "lifecycle.h"
//#include "consts.h"
//#include "hashtable.h"
//#include "net.h"
//
//enum _NetMessageFlag : byte {
//    NET_MESSAGE_FLAG_BROADCAST_HOST_DISCOVERY
//};
//
//typedef struct packed {
//    union {
//        const NetMessagePayload;
//        struct {
//            const char greeting[12];
//            const byte version;
//            const byte masterSessionSealPublicKey[CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE];
//        };
//    };
//} HostDiscoveryBroadcastPayload;
//
//typedef struct packed {
//    union {
//        const NetMessagePayload;
//        const byte masterSessionSealPublicKey[CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE];
//    };
//} ConnectionHelloPayload;
//
//typedef struct {
//    const int address;
//    const byte masterSessionSealPublicKey[CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE];
//    SDLNet_StreamSocket* const socket;
//} Connection;
//
//// everything time-related is in milliseconds
//static const short SUBNET_BROADCAST_SOCKET_PORT = 8080, SUBNET_CONNECTIONS_LISTENER_SERVER_PORT = 8081;
//static const int MESSAGE_RECEIVE_TIME_WINDOW = 100;
//static const int FETCH_SUBNETS_HOST_ADDRESSES_PERIOD = 100, SUBNET_BROADCAST_SEND_PERIOD = 1000, SUBNET_BROADCAST_RECEIVE_PERIOD = 250, ACCEPT_SUBNET_CONNECTIONS_PERIOD = 100;
//static const int UDP_PACKET_MAX_SIZE = 512, TCP_PACKET_MAX_SIZE = 512;
//#define GREETING constsConcatenateTitleWith(" ping")
//
//static atomic bool gInitialized = false;
//static SDL_Mutex* gMutex = nullptr;
//
//static List* gSubnetsHostsAddressesList = nullptr; // <int>
//static int gSelectedSubnetHostAddress = 0; // if not zero then a subnet is being processed
//static SDL_Mutex* gSubnetProcessingMutex = nullptr;
//
//static SDLNet_DatagramSocket* gSubnetBroadcastSocket = nullptr;
//static SDLNet_Server* gSubnetConnectionsListenerServer = nullptr; // it's a socket actually
//static Hashtable* gConnectionsHashtable = nullptr; // <int - address, Connection*>
//
//void netInit(void) {
//    assert(lifecycleInitialized() && !gInitialized);
//    gInitialized = true;
//
//    assert(SDLNet_Init());
//
//    assert(gMutex = SDL_CreateMutex());
//    assert(gSubnetProcessingMutex = SDL_CreateMutex());
//
//    gSubnetsHostsAddressesList = listCreate(false, nullptr);
//}
//
//bool netInitialized(void) {
//    return gInitialized;
//}
//
//static struct ExtractAddressResult {const int address; const bool ipv4;} extractAddress(SDLNet_Address* const address) {
//    const struct addrinfo* const info = *(struct addrinfo**) ((void*) address + 32);
//    return (struct ExtractAddressResult) {
//        (int) swapBytes(*(int*) (info->ai_addr->sa_data + 2)),
//        info->ai_addr->sa_family == AF_INET
//    };
//}
//
//static void fetchSubnetsHostsAddresses(void) {
//    SDL_LockMutex(gMutex);
//    listClear(gSubnetsHostsAddressesList);
//
//    int numberOfAddresses = 0;
//    SDLNet_Address** const addresses = SDLNet_GetLocalAddresses(&numberOfAddresses);
//    assert(numberOfAddresses && addresses);
//
//    for (int i = 0; i < numberOfAddresses; i++) {
//        const struct ExtractAddressResult result = extractAddress(addresses[i]);
//        if (!result.ipv4 || result.address == INADDR_LOOPBACK) continue;
//        listAddBack(gSubnetsHostsAddressesList, (void*) (long) result.address);
//    }
//
//    SDLNet_FreeLocalAddresses(addresses);
//    SDL_UnlockMutex(gMutex);
//}
//
//List* nullable netSubnetsHostsAddresses(void) {
//    SDL_LockMutex(gMutex);
//    List* const new = listCopy(gSubnetsHostsAddressesList, false, nullptr);
//    SDL_UnlockMutex(gMutex);
//    return new;
//}
//
//void netAddressToString(char* const buffer, const int address) {
//    assert(lifecycleInitialized() && gInitialized);
//    xmemset(buffer, 0, NET_ADDRESS_STRING_SIZE);
//    const byte count = SDL_snprintf(
//        buffer,
//        NET_ADDRESS_STRING_SIZE,
//        "%u.%u.%u.%u",
//        (address >> 24) & 0xff, (address >> 16) & 0xff, (address >> 8) & 0xff, address & 0xff
//    );
//    assert(count > 0 && count < NET_ADDRESS_STRING_SIZE);
//}
//
//static SDLNet_Address* resolveAddress(const int address) {
//    char host[NET_ADDRESS_STRING_SIZE];
//    netAddressToString(host, address);
//
//    SDLNet_Address* const resolvedAddress = SDLNet_ResolveHostname(host);
//    assert(resolvedAddress);
//    assert(SDLNet_WaitUntilResolved(resolvedAddress, -1) == 1);
//    return resolvedAddress;
//}
//
//static void destroyConnection(void* const connection) {
//    SDLNet_DestroyStreamSocket(((Connection*) connection)->socket);
//    xfree(connection);
//}
//
//void netStartBroadcastingAndListeningSubnet(const int subnetHostAddress) { // TODO: rename to (start|stop)SubnetProcessing
//    assert(lifecycleInitialized() && gInitialized);
//    assert(subnetHostAddress);
//
//    SDL_LockMutex(gSubnetProcessingMutex);
//    assert(!gSelectedSubnetHostAddress && !gSubnetBroadcastSocket && !gSubnetConnectionsListenerServer && !gConnectionsHashtable);
//
//    gSelectedSubnetHostAddress = subnetHostAddress;
//
//    SDLNet_Address* const address = resolveAddress(gSelectedSubnetHostAddress);
//    assert(gSubnetBroadcastSocket = SDLNet_CreateDatagramSocket(address, SUBNET_BROADCAST_SOCKET_PORT));
//    assert(gSubnetConnectionsListenerServer = SDLNet_CreateServer(address, SUBNET_CONNECTIONS_LISTENER_SERVER_PORT));
//    SDLNet_UnrefAddress(address);
//
//    assert(!setsockopt(
//        *(int*) ((void*) gSubnetBroadcastSocket + 20),
//        SOL_SOCKET,
//        SO_BROADCAST,
//        (int[1]) {1},
//        sizeof(int)
//    ));
//
//    gConnectionsHashtable = hashtableCreate(false, destroyConnection);
//
//    SDL_UnlockMutex(gSubnetProcessingMutex);
//}
//
//void netStopBroadcastingAndListeningSubnet(void) {
//    assert(lifecycleInitialized() && gInitialized);
//
//    SDL_LockMutex(gSubnetProcessingMutex);
//    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket && gSubnetConnectionsListenerServer && gConnectionsHashtable);
//
//    gSelectedSubnetHostAddress = 0;
//
//    SDLNet_DestroyDatagramSocket(gSubnetBroadcastSocket);
//    gSubnetBroadcastSocket = nullptr;
//
//    SDLNet_DestroyServer(gSubnetConnectionsListenerServer);
//    gSubnetConnectionsListenerServer = nullptr;
//
//    hashtableDestroy(gConnectionsHashtable);
//    gConnectionsHashtable = nullptr;
//
//    SDL_UnlockMutex(gSubnetProcessingMutex);
//}
//
//static void broadcastSubnetForHosts(void) {
//    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket);
//
//    const int messageSize = sizeof(NetMessage) + sizeof(HostDiscoveryBroadcastPayload);
//    staticAssert(messageSize <= UDP_PACKET_MAX_SIZE);
//
//    NetMessage* const message = xalloca(messageSize);
//    unconst(message->flag) = NET_MESSAGE_FLAG_BROADCAST_HOST_DISCOVERY;
//    unconst(message->timestamp) = lifecycleCurrentTimeMillis();
//    unconst(message->index) = 0;
//    unconst(message->count) = 1;
//    unconst(message->from) = gSelectedSubnetHostAddress;
//    unconst(message->to) = NET_MESSAGE_TO_EVERYONE;
//    unconst(message->size) = sizeof(HostDiscoveryBroadcastPayload);
//
//    HostDiscoveryBroadcastPayload* const payload = (void*) message->payload;
//    strncpy((char*) payload->greeting, GREETING, sizeof payload->greeting);
//    unconst(payload->version) = 1;
//    xmemcpy((byte*) payload->masterSessionSealPublicKey, cryptoMasterSessionSealPublicKey(), CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE);
//
//    cryptoMasterSign(
//        (byte*) message,
//        messageSize - CRYPTO_SIGNATURE_SIZE,
//        (byte*) message->signature
//    );
//
//    SDLNet_Address* const address = resolveAddress(message->to);
//
//    SDL_LockMutex(gMutex); // TODO: test when these sockets (broadcast and connects) (not the remote ones, exactly these) get disconnected, like when the system gets disconnected from lan/wifi - this would cause asserts failures
//    assert(SDLNet_SendDatagram(gSubnetBroadcastSocket, address, SUBNET_BROADCAST_SOCKET_PORT, message, messageSize));
//    SDL_UnlockMutex(gMutex); // TODO: instead of asserting sockets operations return if they fail and do cleanup and this would indicate disconnection and network monitoring stop so call stopSubnetListeningAndBroadcasting
//
//    SDLNet_UnrefAddress(address);
//}
//
//static inline bool checkSignedMessage(const NetMessage* const message, const int payloadSize) {
//    return cryptoCheckMasterSigned(
//        (const byte*) message,
//        (int) sizeof(NetMessage) + payloadSize - CRYPTO_SIGNATURE_SIZE,
//        message->signature
//    );
//}
//
//static void listenSubnetForBroadcasts(void) {
//    assert(gSelectedSubnetHostAddress && gSubnetBroadcastSocket);
//
//    SDLNet_Datagram* datagram;
//
//    SDL_LockMutex(gMutex);
//    assert(SDLNet_ReceiveDatagram(gSubnetBroadcastSocket, &datagram));
//    SDL_UnlockMutex(gMutex);
//
//    if (!datagram) return;
//
//    if (datagram->buflen == sizeof(NetMessage) + sizeof(HostDiscoveryBroadcastPayload)) // as anyone can send anything anywhere over udp
//        assert(checkSignedMessage((NetMessage*) datagram->buf, sizeof(HostDiscoveryBroadcastPayload))); // TODO: return if signature check fails
//
//    // TODO: try to connect to that host if haven't already
//
//    SDLNet_DestroyDatagram(datagram);
//}
//
//static bool readFromTCPSocket(SDLNet_StreamSocket* const socket, void* const buffer, const int size) {
//    const unsigned long startedMillis = lifecycleCurrentTimeMillis();
//    int totalBytesRead = 0;
//
//    for (
//        int currentBytesRead;
//        (currentBytesRead = SDLNet_ReadFromStreamSocket(socket, buffer + totalBytesRead, size - totalBytesRead));
//        totalBytesRead += currentBytesRead
//    ) {
//        if (lifecycleCurrentTimeMillis() - startedMillis >= MESSAGE_RECEIVE_TIME_WINDOW)
//            break;
//    }
//
//    return totalBytesRead == size;
//}
//
//static void acceptConnections(void) {
//    assert(gSelectedSubnetHostAddress && gSubnetConnectionsListenerServer && gConnectionsHashtable);
//    SDLNet_StreamSocket* connectionSocket;
//
//    while (true) {
//        SDL_LockMutex(gMutex);
//        assert(SDLNet_AcceptClient(gSubnetConnectionsListenerServer, &connectionSocket));
//        SDL_UnlockMutex(gMutex);
//
//        if (!connectionSocket) return;
//
//        SDLNet_Address* const addr = SDLNet_GetStreamSocketAddress(connectionSocket);
//        assert(addr);
//        const int address = extractAddress(addr).address;
//        SDLNet_UnrefAddress(addr);
//
//        SDL_LockMutex(gMutex);
//        const Connection* const connection = hashtableGet(gConnectionsHashtable, hashtableHashPrimitive(address));
//        SDL_UnlockMutex(gMutex);
//        if (connection) {
//            assert(connection->address == address);
//            SDLNet_DestroyStreamSocket(connectionSocket); // connection is already established
//            continue;
//        }
//
//        const int messageSize = sizeof(NetMessage) + sizeof(ConnectionHelloPayload);
//        staticAssert(messageSize <= TCP_PACKET_MAX_SIZE);
//        NetMessage* const message = xalloca(messageSize);
//
//        if (!readFromTCPSocket(connectionSocket, message, messageSize)) {
//            SDLNet_DestroyStreamSocket(connectionSocket);
//            continue;
//        }
//
//        if (message->from != address) {
//            SDLNet_DestroyStreamSocket(connectionSocket);
//            continue;
//        }
//
//        const ConnectionHelloPayload* const payload = (void*) message->payload;
//
//        if (!checkSignedMessage(message, sizeof(ConnectionHelloPayload))) {
//            SDLNet_DestroyStreamSocket(connectionSocket);
//            continue;
//        }
//
//        if (!SDLNet_WriteToStreamSocket(connectionSocket, /*TODO: no, create a new secret key for each p2p connection, use secret stream*/ cryptoMasterSessionSealPublicKey(), CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE)) {
//            SDLNet_DestroyStreamSocket(connectionSocket);
//            continue;
//        }
//
//        if (SDLNet_WaitUntilStreamSocketDrained(connectionSocket, MESSAGE_RECEIVE_TIME_WINDOW) != 0) {
//            SDLNet_DestroyStreamSocket(connectionSocket);
//            continue;
//        }
//
//        Connection* const newConnection = xmalloc(sizeof *newConnection);
//        unconst(newConnection->address) = address;
//        xmemcpy((void*) newConnection->masterSessionSealPublicKey, payload->masterSessionSealPublicKey, CRYPTO_ENCRYPT_PUBLIC_SECRET_KEY_SIZE);
//        unconst(newConnection->socket) = connectionSocket;
//
//        SDL_LockMutex(gMutex);
//        hashtablePut(gConnectionsHashtable, hashtableHashPrimitive(address), newConnection);
//        SDL_UnlockMutex(gMutex);
//    }
//}
//
//static void runPeriodically(const unsigned long currentMillis, unsigned long* const lastRunMillis, const int period, void (* const action)(void)) {
//    if (currentMillis - *lastRunMillis < (unsigned) period) return;
//    *lastRunMillis = currentMillis;
//    action();
//}
//
//void netLoop(void) {
//    assert(lifecycleInitialized() && gInitialized);
//
//    const unsigned long currentMillis = lifecycleCurrentTimeMillis();
//    static unsigned long lastSubnetsHostsAddressesFetch = 0, lastBroadcastSend = 0, lastBroadcastReceive = 0, lastClientsAccept = 0;
//
//    SDL_LockMutex(gSubnetProcessingMutex);
//    if (gSelectedSubnetHostAddress) {
//        // TODO: periodically check gConnectionsHashtable for disconnected connections and remove them --- no need as it would be known for a socket to be disconnected when the future messages querying loop would try to access that socket
//        runPeriodically(currentMillis, &lastBroadcastSend, SUBNET_BROADCAST_SEND_PERIOD, broadcastSubnetForHosts);
//        runPeriodically(currentMillis, &lastBroadcastReceive, SUBNET_BROADCAST_RECEIVE_PERIOD, listenSubnetForBroadcasts);
//        runPeriodically(currentMillis, &lastClientsAccept, ACCEPT_SUBNET_CONNECTIONS_PERIOD, acceptConnections);
//        SDL_UnlockMutex(gSubnetProcessingMutex);
//    } else {
//        SDL_UnlockMutex(gSubnetProcessingMutex);
//        runPeriodically(currentMillis, &lastSubnetsHostsAddressesFetch, FETCH_SUBNETS_HOST_ADDRESSES_PERIOD, fetchSubnetsHostsAddresses);
//    }
//}
//
//void netQuit(void) {
//    assert(gInitialized);
//
//    // not wrapped with gMutex as at this point only the main thread is running, and it has waited until others finished
//    if (gSelectedSubnetHostAddress) netStopBroadcastingAndListeningSubnet();
//
//    gInitialized = false;
//
//    listDestroy(gSubnetsHostsAddressesList);
//
//    SDL_DestroyMutex(gSubnetProcessingMutex);
//    SDL_DestroyMutex(gMutex);
//
//    SDLNet_Quit();
//}
