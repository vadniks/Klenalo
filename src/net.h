
#pragma once

#include "list.h"

extern const int NET_ADDRESS_STRING_SIZE; // null terminator included

void netInit(void);
bool netInitialized(void);
List* nullable netSubnets(void); // <int>
void netAddressToString(char* const buffer, const int address);
void netStartBroadcastingAndListeningSubnet(const int subnet);
void netStopBroadcastingAndListeningSubnet(void); // called in quit automatically
void netLoop(void);
void netQuit(void);
