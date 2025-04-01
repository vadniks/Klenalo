
#pragma once

#include "list.h"
#include "crypto.h"

enum _NetMessageFlag : byte;
typedef enum _NetMessageFlag NetMessageFlag;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
typedef struct packed {
    const byte payload[];
} NetMessagePayload;
#pragma clang diagnostic pop

typedef struct packed {
    const NetMessageFlag flag;
    const unsigned long timestamp;
    const int index, count, from, to, size;
    __attribute_used__ const NetMessagePayload;
    const byte signature[CRYPTO_SIGNATURE_SIZE];
} NetMessage;

enum : int {
    NET_ADDRESS_STRING_SIZE = 3 * 4 + 3 + 1, // xxx.xxx.xxx.xxx\0
    NET_MESSAGE_TO_EVERYONE = -1 // INADDR_BROADCAST
};

void netInit(void);
bool netInitialized(void);
List* nullable netSubnetsHostsAddresses(void); // <int>
void netAddressToString(char* const buffer, const int address);
void netStartBroadcastingAndListeningSubnet(const int subnetHostAddress);
void netStopBroadcastingAndListeningSubnet(void); // called in quit automatically
void netLoop(void);
void netQuit(void);
