
#pragma once

#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <SDL3_net/SDL_net.h>
#include "../../common/defs.h"

typedef union {
    struct in6_addr scalar;
    vec16aw vector;
} In6Addr;

typedef struct {
    char interface[IF_NAMESIZE];
    unsigned interfaceIndex;
    In6Addr
        slaacAddress, // for user to see
        linkLocalAddress; // for us to use
    unsigned mask;
} Address;
