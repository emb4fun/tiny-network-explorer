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
#include "tnp_platform.h"

#include <string.h>

static int g_platform_initialized;

int tnp_platform_init(void)
{
    WSADATA wsa_data;

    if (g_platform_initialized) {
        return 0;
    }

    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        return -1;
    }

    g_platform_initialized = 1;
    return 0;
}

void tnp_platform_cleanup(void)
{
    if (!g_platform_initialized) {
        return;
    }

    WSACleanup();
    g_platform_initialized = 0;
}

int tnp_platform_enumerate_interfaces(tnp_platform_iface_t* ifaces, int max_count)
{
    INTERFACE_INFO interface_list[TNP_MAX_INTERFACES];
    struct sockaddr_in* address;
    SOCKET probe_socket;
    unsigned long bytes_returned;
    int interface_count;
    int index;
    int result;
    int saved_count;

    if (ifaces == NULL || max_count <= 0) {
        return 0;
    }

    saved_count = 0;
    probe_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (probe_socket == INVALID_SOCKET) {
        return 0;
    }

    result = WSAIoctl(
        probe_socket,
        SIO_GET_INTERFACE_LIST,
        NULL,
        0,
        interface_list,
        (DWORD)sizeof(interface_list),
        &bytes_returned,
        NULL,
        NULL);

    closesocket(probe_socket);

    if (result != 0) {
        return 0;
    }

    interface_count = (int)(bytes_returned / sizeof(INTERFACE_INFO));
    for (index = 0; index < interface_count && saved_count < max_count; ++index) {
        address = (struct sockaddr_in*)&interface_list[index].iiAddress;
        if (ntohl(address->sin_addr.s_addr) == 0x7F000001u) {
            continue;
        }

        ifaces[saved_count].socket = TNP_INVALID_SOCKET;
        ifaces[saved_count].address_host = ntohl(address->sin_addr.s_addr);
        saved_count++;
    }

    return saved_count;
}

int tnp_platform_setup_udp_socket(tnp_platform_iface_t* iface, uint16_t port)
{
    struct sockaddr_in bind_address;
    int broadcast_enabled;
    int result;

    if (iface == NULL) {
        return -1;
    }

    if (iface->socket != TNP_INVALID_SOCKET) {
        closesocket(iface->socket);
    }

    iface->socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (iface->socket == INVALID_SOCKET) {
        return -1;
    }

    broadcast_enabled = TRUE;
    result = setsockopt(
        iface->socket,
        SOL_SOCKET,
        SO_BROADCAST,
        (const char*)&broadcast_enabled,
        (int)sizeof(broadcast_enabled));
    if (result != 0) {
        closesocket(iface->socket);
        iface->socket = TNP_INVALID_SOCKET;
        return -1;
    }

    memset(&bind_address, 0, sizeof(bind_address));
    bind_address.sin_family = AF_INET;
    bind_address.sin_addr.s_addr = htonl(iface->address_host);
    bind_address.sin_port = htons(port);

    result = bind(iface->socket, (struct sockaddr*)&bind_address, (int)sizeof(bind_address));
    if (result != 0) {
        closesocket(iface->socket);
        iface->socket = TNP_INVALID_SOCKET;
        return -1;
    }

    return 0;
}

void tnp_platform_close_socket(tnp_socket_t socket)
{
    if (socket != TNP_INVALID_SOCKET) {
        closesocket(socket);
    }
}

int tnp_platform_set_nonblocking(tnp_socket_t socket)
{
    u_long mode;

    if (socket == TNP_INVALID_SOCKET) {
        return -1;
    }

    mode = 1;
    return (ioctlsocket(socket, FIONBIO, &mode) == 0) ? 0 : -1;
}

int tnp_platform_send_broadcast(
    const tnp_platform_iface_t* iface,
    uint16_t port,
    const void* data,
    size_t length)
{
    struct sockaddr_in destination;
    int result;

    if (iface == NULL || iface->socket == TNP_INVALID_SOCKET || data == NULL || length == 0) {
        return -1;
    }

    memset(&destination, 0, sizeof(destination));
    destination.sin_family = AF_INET;
    destination.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    destination.sin_port = htons(port);

    result = sendto(
        iface->socket,
        (const char*)data,
        (int)length,
        0,
        (struct sockaddr*)&destination,
        (int)sizeof(destination));

    return (result == SOCKET_ERROR) ? -1 : 0;
}

int tnp_platform_recvfrom(
    tnp_socket_t socket,
    void* buffer,
    size_t buffer_length,
    struct sockaddr_in* source,
    int* source_length)
{
    int bytes_received;

    if (socket == TNP_INVALID_SOCKET || buffer == NULL || source == NULL || source_length == NULL) {
        return -1;
    }

    bytes_received = recvfrom(
        socket,
        (char*)buffer,
        (int)buffer_length,
        0,
        (struct sockaddr*)source,
        source_length);

    if (bytes_received == SOCKET_ERROR) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            return 0;
        }
        return -1;
    }

    return bytes_received;
}

int tnp_platform_poll_readable(tnp_socket_t socket, int timeout_ms)
{
    fd_set read_set;
    struct timeval timeout;
    int result;

    if (socket == TNP_INVALID_SOCKET) {
        return 0;
    }

    FD_ZERO(&read_set);
    FD_SET(socket, &read_set);

    timeout.tv_sec = timeout_ms / 1000;
    timeout.tv_usec = (timeout_ms % 1000) * 1000;

    result = select(0, &read_set, NULL, NULL, &timeout);
    if (result <= 0) {
        return 0;
    }

    return FD_ISSET(socket, &read_set) ? 1 : 0;
}
