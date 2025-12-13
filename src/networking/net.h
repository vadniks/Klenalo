//
//#pragma once
//
//#include "list.h"
//#include "crypto.h"
//
//typedef enum _NetMessageFlag : byte NetMessageFlag;
//
//typedef struct packed {
//    const byte payload[];
//} used NetMessagePayload;
//
//typedef struct packed {
//    const NetMessageFlag flag;
//    const unsigned long timestamp;
//    const int index, count, from, to, size;
//    used const NetMessagePayload;
//    const byte signature[CRYPTO_SIGNATURE_SIZE];
//} NetMessage;
//
//enum : int {
//    NET_ADDRESS_STRING_SIZE = 3 * 4 + 3 + 1, // xxx.xxx.xxx.xxx\0
//    NET_MESSAGE_TO_EVERYONE = -1 // INADDR_BROADCAST
//};
//
////void netInit(void);
////bool netInitialized(void);
////List* nullable netSubnetsHostsAddresses(void); // <int>
////void netAddressToString(char* const buffer, const int address);
////void netStartBroadcastingAndListeningSubnet(const int subnetHostAddress);
////void netStopBroadcastingAndListeningSubnet(void); // called in quit automatically
////void netLoop(void);
////void netQuit(void);
