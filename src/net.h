
#pragma once

#include "list.h"

enum _NetMessageFlag : byte;
typedef enum _NetMessageFlag NetMessageFlag;

typedef struct packed {
    const NetMessageFlag flag;
    const unsigned long timestamp;
    const int index, count, from, to, size;
    const byte payload[/*size*/];
} NetMessage;

extern const int NET_ADDRESS_STRING_SIZE; // null terminator included

void netInit(void);
bool netInitialized(void);
List* nullable netSubnetsHostsAddresses(void); // <int>
void netAddressToString(char* const buffer, const int address);
void netStartBroadcastingAndListeningSubnet(const int subnetHostAddress);
void netStopBroadcastingAndListeningSubnet(void); // called in quit automatically
void netLoop(void);
void netQuit(void);
