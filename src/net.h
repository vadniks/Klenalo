
#pragma once

#include "list.h"

typedef struct {
    const char name[16]; // less than NI_MAXHOST
    const int address;
    const byte mask; // number of unit bits in a mask
    const int broadcast;
    const int host; // address of this host
    const int hostsCount;
    const bool private;
    const bool running;
} NetSubnet;

extern const int NET_ADDRESS_STRING_SIZE; // null terminator included

void netInit(void);
bool netInitialized(void);
List* nullable netSubnets(void); // <NetSubnet*> caller must destroy the list, the embedded deallocator will free the items
void netAddressToString(char* const buffer, const int address);
void netStartBroadcastingAndListeningSubnet(const NetSubnet* const subnet);
void netStopBroadcastingAndListeningSubnet(void); // called in quit automatically
void netLoop(void);
void netQuit(void);
