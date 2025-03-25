
#pragma once

#include "list.h"

enum _NetMessageFlag : byte;
typedef enum _NetMessageFlag NetMessageFlag;

typedef struct packed {
    const byte payload[];
} NetMessagePayload;

typedef struct packed {
    const NetMessageFlag flag;
    const unsigned long timestamp;
    const int index, count, from, to, size;
    const NetMessagePayload;
} NetMessage;

enum : int {
    NET_ADDRESS_STRING_SIZE = 3 * 4 + 3 + 1, // xxx.xxx.xxx.xxx\0
    NET_MESSAGE_SIZE = sizeof(NetMessage),
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
