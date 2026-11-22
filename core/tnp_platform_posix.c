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
#if defined(__linux__)
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#endif

#include "tnp_platform.h"

#include <errno.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>
#include <poll.h>

static int g_platform_initialized;

int tnp_platform_init(void)
{
    g_platform_initialized = 1;
    return 0;
}

void tnp_platform_cleanup(void)
{
    g_platform_initialized = 0;
}

int tnp_platform_enumerate_interfaces(tnp_platform_iface_t* ifaces, int max_count)
{
    struct ifaddrs* interface_addresses;
    struct ifaddrs* current;
    int saved_count;

    if (ifaces == NULL || max_count <= 0) {
        return 0;
    }

    saved_count = 0;
    if (getifaddrs(&interface_addresses) != 0) {
        return 0;
    }

    for (current = interface_addresses; current != NULL && saved_count < max_count; current = current->ifa_next) {
        struct sockaddr_in* address;

        if (current->ifa_addr == NULL || current->ifa_addr->sa_family != AF_INET) {
            continue;
        }

        if ((current->ifa_flags & IFF_UP) == 0 || (current->ifa_flags & IFF_LOOPBACK) != 0) {
            continue;
        }

        address = (struct sockaddr_in*)current->ifa_addr;
        if (ntohl(address->sin_addr.s_addr) == 0x7F000001u) {
            continue;
        }

        ifaces[saved_count].socket = TNP_INVALID_SOCKET;
        ifaces[saved_count].address_host = ntohl(address->sin_addr.s_addr);
        ifaces[saved_count].ifname[0] = '\0';
        if (current->ifa_name != NULL) {
            strncpy(ifaces[saved_count].ifname, current->ifa_name, sizeof(ifaces[saved_count].ifname) - 1);
            ifaces[saved_count].ifname[sizeof(ifaces[saved_count].ifname) - 1] = '\0';
        }
        saved_count++;
    }

    freeifaddrs(interface_addresses);
    return saved_count;
}

int tnp_platform_setup_udp_socket(tnp_platform_iface_t* iface, uint16_t port)
{
    struct sockaddr_in bind_address;
    int broadcast_enabled;
    int reuse_enabled;
    int result;

    if (iface == NULL) {
        return -1;
    }

    if (iface->socket != TNP_INVALID_SOCKET) {
        close(iface->socket);
    }

    iface->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (iface->socket == TNP_INVALID_SOCKET) {
        return -1;
    }

    reuse_enabled = 1;
    result = setsockopt(
        iface->socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &reuse_enabled,
        (socklen_t)sizeof(reuse_enabled));
    if (result != 0) {
        close(iface->socket);
        iface->socket = TNP_INVALID_SOCKET;
        return -1;
    }

    broadcast_enabled = 1;
    result = setsockopt(
        iface->socket,
        SOL_SOCKET,
        SO_BROADCAST,
        &broadcast_enabled,
        (socklen_t)sizeof(broadcast_enabled));
    if (result != 0) {
        close(iface->socket);
        iface->socket = TNP_INVALID_SOCKET;
        return -1;
    }

#if defined(__linux__)
    if (iface->ifname[0] != '\0') {
        (void)setsockopt(
            iface->socket,
            SOL_SOCKET,
            SO_BINDTODEVICE,
            iface->ifname,
            (socklen_t)(strlen(iface->ifname) + 1u));
    }
#endif

    /*
     * Tiny devices reply to 255.255.255.255:54321. Linux does not deliver
     * limited broadcasts to sockets bound to a unicast address (Windows does).
     * Use INADDR_ANY (see SO_BINDTODEVICE above).
     */
    memset(&bind_address, 0, sizeof(bind_address));
    bind_address.sin_family = AF_INET;
    bind_address.sin_addr.s_addr = htonl(INADDR_ANY);
    bind_address.sin_port = htons(port);

    result = bind(iface->socket, (struct sockaddr*)&bind_address, (socklen_t)sizeof(bind_address));
    if (result != 0) {
        close(iface->socket);
        iface->socket = TNP_INVALID_SOCKET;
        return -1;
    }

    return 0;
}

void tnp_platform_close_socket(tnp_socket_t socket)
{
    if (socket != TNP_INVALID_SOCKET) {
        close(socket);
    }
}

int tnp_platform_set_nonblocking(tnp_socket_t socket)
{
    int flags;

    if (socket == TNP_INVALID_SOCKET) {
        return -1;
    }

    flags = fcntl(socket, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }

    return (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == 0) ? 0 : -1;
}

int tnp_platform_send_broadcast(
    const tnp_platform_iface_t* iface,
    uint16_t port,
    const void* data,
    size_t length)
{
    struct sockaddr_in destination;
    ssize_t bytes_sent;

    if (iface == NULL || iface->socket == TNP_INVALID_SOCKET || data == NULL || length == 0) {
        return -1;
    }

    memset(&destination, 0, sizeof(destination));
    destination.sin_family = AF_INET;
    destination.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    destination.sin_port = htons(port);

    bytes_sent = sendto(
        iface->socket,
        data,
        length,
        0,
        (struct sockaddr*)&destination,
        (socklen_t)sizeof(destination));

    return (bytes_sent < 0) ? -1 : 0;
}

int tnp_platform_recvfrom(
    tnp_socket_t socket,
    void* buffer,
    size_t buffer_length,
    struct sockaddr_in* source,
    int* source_length)
{
    socklen_t address_length;
    ssize_t bytes_received;

    if (socket == TNP_INVALID_SOCKET || buffer == NULL || source == NULL || source_length == NULL) {
        return -1;
    }

    address_length = (socklen_t)*source_length;
    bytes_received = recvfrom(
        socket,
        buffer,
        buffer_length,
        0,
        (struct sockaddr*)source,
        &address_length);

    if (bytes_received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        return -1;
    }

    *source_length = (int)address_length;
    return (int)bytes_received;
}

int tnp_platform_poll_readable(tnp_socket_t socket, int timeout_ms)
{
    struct pollfd poll_item;
    int result;

    if (socket == TNP_INVALID_SOCKET) {
        return 0;
    }

    poll_item.fd = socket;
    poll_item.events = POLLIN;
    poll_item.revents = 0;

    result = poll(&poll_item, 1, timeout_ms);
    if (result <= 0) {
        return 0;
    }

    return (poll_item.revents & POLLIN) ? 1 : 0;
}
