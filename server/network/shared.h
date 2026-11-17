
#pragma once

#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <SDL3_net/SDL_net.h>
#include "../../common/defs.h"

enum {
    MAX_UDP_PAYLOAD_SIZE = 1400,
    MAX_TCP_PAYLOAD_SIZE = 1400
};

typedef union {
    struct in6_addr native;
    vec16aw vectorized;
} In6Addr;

typedef struct {
    char name[IF_NAMESIZE];
    unsigned index;
    In6Addr linkLocalAddress;
    unsigned mask;
} Interface;

typedef struct {
    In6Addr linkLocal;
    unsigned interfaceIndex;
} Address;

struct NET_Address {
    char* hostname;
    char* human_readable;
    char* errstr;
    SDL_AtomicInt refcount;
    NET_Status status;
    struct addrinfo* ainfo;
    NET_Address* resolver_next;
};
