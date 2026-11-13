
#pragma once

#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <SDL3_net/SDL_net.h>
#include "../../common/defs.h"

typedef union {
    struct in6_addr native;
    vec16aw vectorized;
} In6Addr;

typedef struct {
    char name[IF_NAMESIZE]; // for user to see
    unsigned index; // for us to use
    In6Addr
        slaacAddress, // for user to see
        linkLocalAddress; // for us to use
    unsigned mask; // for user to see
} Interface;

typedef struct {
    vec16aw linkLocal;
    unsigned interfaceIndex;
} Address;
