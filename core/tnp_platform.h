/**************************************************************************
*  Copyright (c) 2026 by Michael Fischer (www.emb4fun.de).
*  All rights reserved.
*
*  Credit goes to the AI assistant that helped convert the legacy version
*  into a wxWidgets application for Linux and Windows.
*
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*  
*  1. Redistributions of source code must retain the above copyright 
*     notice, this list of conditions and the following disclaimer.
*
*  2. Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in the 
*     documentation and/or other materials provided with the distribution.
*
*  3. Neither the name of the author nor the names of its contributors may 
*     be used to endorse or promote products derived from this software 
*     without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
*  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
*  THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
*  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
*  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS 
*  OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
*  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF 
*  THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
*  SUCH DAMAGE.
**************************************************************************/
#ifndef TNP_PLATFORM_H
#define TNP_PLATFORM_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET tnp_socket_t;
#define TNP_INVALID_SOCKET INVALID_SOCKET
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
typedef int tnp_socket_t;
#define TNP_INVALID_SOCKET (-1)
#endif

#define TNP_MAX_INTERFACES 8

typedef struct tnp_platform_iface {
    tnp_socket_t socket;
    uint32_t address_host;
#ifndef _WIN32
    char ifname[32];
#endif
} tnp_platform_iface_t;

int tnp_platform_init(void);
void tnp_platform_cleanup(void);

int tnp_platform_enumerate_interfaces(tnp_platform_iface_t* ifaces, int max_count);

int tnp_platform_setup_udp_socket(tnp_platform_iface_t* iface, uint16_t port);

void tnp_platform_close_socket(tnp_socket_t socket);

int tnp_platform_set_nonblocking(tnp_socket_t socket);

int tnp_platform_send_broadcast(
    const tnp_platform_iface_t* iface,
    uint16_t port,
    const void* data,
    size_t length);

int tnp_platform_recvfrom(
    tnp_socket_t socket,
    void* buffer,
    size_t buffer_length,
    struct sockaddr_in* source,
    int* source_length);

int tnp_platform_poll_readable(tnp_socket_t socket, int timeout_ms);

#endif /* TNP_PLATFORM_H */
