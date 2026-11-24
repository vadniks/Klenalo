// Copyright (C) 2026 Vadim Nikolaev (https://github.com/vadniks)
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <ifaddrs.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <asm-generic/socket.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "../../common/defs.h"

#define asSockaddrIn6(x) ((struct sockaddr_in6*) x)

enum {
    MAX_UDP_PAYLOAD_SIZE = 1400,
    MAX_TCP_PAYLOAD_SIZE = 1400
};

typedef union {
    struct in6_addr native;
    vec16uw vectorized;
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

// struct NET_Address {
//     char* hostname;
//     char* human_readable;
//     char* errstr;
//     SDL_AtomicInt refcount;
//     NET_Status status;
//     struct addrinfo* ainfo;
//     NET_Address* resolver_next;
// };
//
// typedef int Socket;
//
// typedef enum NET_SocketType {
//     SOCKETTYPE_STREAM,
//     SOCKETTYPE_DATAGRAM,
//     SOCKETTYPE_SERVER
// } NET_SocketType;
//
// typedef struct NET_DatagramSocketHandle {
//     Socket handle;
//     int family;
//     int protocol;
//     NET_Address *broadcast;
// } NET_DatagramSocketHandle;
//
// struct NET_DatagramSocket {
//     NET_SocketType socktype;
//     NET_Address *addr;  // bound to this address (NULL for any).
//     Uint16 port;
//     int percent_loss;
//     Uint8 recv_buffer[64*1024];
//     NET_Address *latest_recv_addrs[64];
//     int latest_recv_addrs_idx;
//     int num_handles;   // for INADDR_ANY things, one handle (etc) per network family.
//     NET_DatagramSocketHandle *handles;
//     NET_DatagramSocketHandle handle_pool[4];
//     NET_Datagram **pending_output;
//     int pending_output_len;
//     int pending_output_allocation;
//     bool allow_broadcast;
// };
