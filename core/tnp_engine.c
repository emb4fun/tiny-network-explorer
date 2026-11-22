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
#include "tnp_engine.h"

#include <stdio.h>
#include <string.h>

#include "tnp_platform.h"

#define TNP_MAX_DEVICES 128

static tnp_event_callback g_callback;
static void* g_user_data;
static int g_running;

static tnp_platform_iface_t g_ifaces[TNP_MAX_INTERFACES];
static int g_iface_count;

static tnp_device_t g_devices[TNP_MAX_DEVICES];
static size_t g_device_count;

static void emit_event(const tnp_event_t* event)
{
    if (g_callback != NULL) {
        g_callback(event, g_user_data);
    }
}

static void emit_error(const char* message)
{
    tnp_event_t event;

    memset(&event, 0, sizeof(event));
    event.type = TNP_EVENT_ERROR;
    event.message = message;
    emit_event(&event);
}

static void ipv4_to_string(uint32_t address_network, char* buffer, size_t buffer_length)
{
#ifdef _WIN32
    struct in_addr addr;
    addr.S_un.S_addr = address_network;
#else
    struct in_addr addr;
    addr.s_addr = address_network;
#endif

    if (inet_ntop(AF_INET, &addr, buffer, (socklen_t)buffer_length) == NULL) {
        buffer[0] = '\0';
    }
}

static int mac_addresses_equal(const uint8_t* left, const uint8_t* right)
{
    return memcmp(left, right, 6) == 0;
}

static int mac_address_is_zero(const uint8_t* mac)
{
    static const uint8_t zero_mac[6] = {0, 0, 0, 0, 0, 0};
    return mac_addresses_equal(mac, zero_mac);
}

static void fill_device_from_setup(const tnp_setup_t* setup, tnp_device_t* device)
{
    memset(device, 0, sizeof(*device));

    device->is_es = (setup->mode == TNP_SETUP_RESPONSE_ES) ? 1u : 0u;
    device->use_dhcp = setup->use_dhcp;
    device->address = setup->address;
    device->mask = setup->mask;
    device->gateway = setup->gateway;

    memcpy(device->mac_address, setup->mac_address, sizeof(device->mac_address));
    memcpy(device->name, setup->name, TNP_MAX_NAME_LEN);
    memcpy(device->location, setup->location, TNP_MAX_LOCATION_LEN);
    memcpy(device->mdns_name, setup->mdns_name, TNP_MAX_MDNS_NAME_LEN);

    snprintf(
        device->display_mac,
        sizeof(device->display_mac),
        "%02X:%02X:%02X:%02X:%02X:%02X",
        setup->mac_address[0],
        setup->mac_address[1],
        setup->mac_address[2],
        setup->mac_address[3],
        setup->mac_address[4],
        setup->mac_address[5]);

    ipv4_to_string(setup->address, device->display_ip, sizeof(device->display_ip));

    snprintf(
        device->display_name,
        sizeof(device->display_name),
        "%s - v%u.%02u",
        setup->name,
        setup->fw_version / 100u,
        setup->fw_version % 100u);

    snprintf(
        device->display_location,
        sizeof(device->display_location),
        "%s",
        setup->location);
}

static int find_device_by_mac(const uint8_t* mac_address)
{
    size_t index;

    for (index = 0; index < g_device_count; ++index) {
        if (mac_addresses_equal(g_devices[index].mac_address, mac_address)) {
            return (int)index;
        }
    }

    return -1;
}

static int setup_packet_is_valid(const tnp_setup_t* setup, int bytes_received)
{
    return bytes_received == (int)sizeof(tnp_setup_t) &&
           setup->magic1 == TNP_HEADER_MAGIC_1 &&
           setup->magic2 == TNP_HEADER_MAGIC_2 &&
           setup->size == (uint16_t)sizeof(tnp_setup_t) &&
           setup->version == TNP_HEADER_VERSION;
}

static void handle_response(const tnp_setup_t* setup)
{
    tnp_event_t event;
    int existing_index;
    tnp_device_t device;

    if (mac_address_is_zero(setup->mac_address)) {
        return;
    }

    if (setup->mode != TNP_SETUP_RESPONSE && setup->mode != TNP_SETUP_RESPONSE_ES) {
        return;
    }

    fill_device_from_setup(setup, &device);
    existing_index = find_device_by_mac(setup->mac_address);

    memset(&event, 0, sizeof(event));
    event.device = device;

    if (existing_index < 0) {
        if (g_device_count >= TNP_MAX_DEVICES) {
            emit_error("Device list is full.");
            return;
        }

        g_devices[g_device_count] = device;
        event.type = TNP_EVENT_DEVICE_ADDED;
        event.index = (int)g_device_count;
        g_device_count++;
    } else {
        g_devices[(size_t)existing_index] = device;
        event.type = TNP_EVENT_DEVICE_UPDATED;
        event.index = existing_index;
    }

    emit_event(&event);
}

static void read_interface(int interface_index)
{
    tnp_setup_t setup;
    struct sockaddr_in source;
    int source_length;
    int bytes_received;

    if (interface_index < 0 || interface_index >= g_iface_count) {
        return;
    }

    source_length = (int)sizeof(source);
    bytes_received = tnp_platform_recvfrom(
        g_ifaces[interface_index].socket,
        &setup,
        sizeof(setup),
        &source,
        &source_length);

    if (bytes_received <= 0) {
        return;
    }

    if (!setup_packet_is_valid(&setup, bytes_received)) {
        return;
    }

    handle_response(&setup);
}

static void clear_devices(void)
{
    tnp_event_t event;

    g_device_count = 0;
    memset(g_devices, 0, sizeof(g_devices));

    memset(&event, 0, sizeof(event));
    event.type = TNP_EVENT_LIST_CLEARED;
    emit_event(&event);
}

int tnp_engine_init(tnp_event_callback callback, void* user_data)
{
    int index;

    if (g_running) {
        return 0;
    }

    if (callback == NULL) {
        return -1;
    }

    if (tnp_platform_init() != 0) {
        return -1;
    }

    g_callback = callback;
    g_user_data = user_data;
    g_device_count = 0;
    g_iface_count = 0;
    memset(g_devices, 0, sizeof(g_devices));
    memset(g_ifaces, 0, sizeof(g_ifaces));

    g_iface_count = tnp_platform_enumerate_interfaces(g_ifaces, TNP_MAX_INTERFACES);
    if (g_iface_count <= 0) {
        tnp_platform_cleanup();
        emit_error("No usable network interfaces found.");
        g_callback = NULL;
        g_user_data = NULL;
        return -1;
    }

    for (index = 0; index < g_iface_count; ++index) {
        if (tnp_platform_setup_udp_socket(&g_ifaces[index], (uint16_t)TNP_UDP_PORT) != 0) {
            int cleanup_index;

            for (cleanup_index = 0; cleanup_index < index; ++cleanup_index) {
                tnp_platform_close_socket(g_ifaces[cleanup_index].socket);
            }
            tnp_platform_cleanup();
            emit_error("Failed to bind UDP sockets.");
            g_callback = NULL;
            g_user_data = NULL;
            return -1;
        }

        if (tnp_platform_set_nonblocking(g_ifaces[index].socket) != 0) {
            int cleanup_index;

            for (cleanup_index = 0; cleanup_index <= index; ++cleanup_index) {
                tnp_platform_close_socket(g_ifaces[cleanup_index].socket);
            }
            tnp_platform_cleanup();
            emit_error("Failed to configure UDP sockets.");
            g_callback = NULL;
            g_user_data = NULL;
            return -1;
        }
    }

    g_running = 1;
    return 0;
}

void tnp_engine_shutdown(void)
{
    int index;

    if (!g_running) {
        return;
    }

    for (index = 0; index < g_iface_count; ++index) {
        tnp_platform_close_socket(g_ifaces[index].socket);
        g_ifaces[index].socket = TNP_INVALID_SOCKET;
    }

    g_iface_count = 0;
    g_device_count = 0;
    g_running = 0;
    g_callback = NULL;
    g_user_data = NULL;

    tnp_platform_cleanup();
}

int tnp_engine_is_running(void)
{
    return g_running;
}

int tnp_engine_search(void)
{
    tnp_setup_t setup;
    int index;

    if (!g_running) {
        return -1;
    }

    clear_devices();

    memset(&setup, 0, sizeof(setup));
    setup.magic1 = TNP_HEADER_MAGIC_1;
    setup.magic2 = TNP_HEADER_MAGIC_2;
    setup.size = (uint16_t)sizeof(setup);
    setup.version = (uint16_t)TNP_HEADER_VERSION;
    setup.mode = (uint8_t)TNP_SETUP_REQUEST;

    for (index = 0; index < g_iface_count; ++index) {
        if (tnp_platform_send_broadcast(
                &g_ifaces[index],
                (uint16_t)TNP_UDP_PORT,
                &setup,
                sizeof(setup)) != 0) {
            emit_error("Failed to send discovery broadcast.");
            return -1;
        }
    }

    return 0;
}

int tnp_engine_update(int device_index, const tnp_device_t* new_settings)
{
    tnp_setup_t setup;
    tnp_event_t event;
    tnp_device_t* device;
    int index;

    if (!g_running || new_settings == NULL) {
        return -1;
    }

    if (device_index < 0 || (size_t)device_index >= g_device_count) {
        return -1;
    }

    device = &g_devices[(size_t)device_index];

    memset(&setup, 0, sizeof(setup));
    setup.magic1 = TNP_HEADER_MAGIC_1;
    setup.magic2 = TNP_HEADER_MAGIC_2;
    setup.size = (uint16_t)sizeof(setup);
    setup.version = (uint16_t)TNP_HEADER_VERSION;
    setup.mode = (uint8_t)TNP_SETUP_SET;
    setup.use_dhcp = (uint8_t)(new_settings->use_dhcp & 0x00FFu);
    setup.address = new_settings->address;
    setup.mask = new_settings->mask;
    setup.gateway = new_settings->gateway;
    memcpy(setup.mac_address, new_settings->mac_address, sizeof(setup.mac_address));
    memcpy(setup.location, new_settings->location, TNP_MAX_LOCATION_LEN);

    for (index = 0; index < g_iface_count; ++index) {
        if (tnp_platform_send_broadcast(
                &g_ifaces[index],
                (uint16_t)TNP_UDP_PORT,
                &setup,
                sizeof(setup)) != 0) {
            emit_error("Failed to send configuration update.");
            return -1;
        }
    }

    device->use_dhcp = new_settings->use_dhcp;
    device->address = new_settings->address;
    device->mask = new_settings->mask;
    device->gateway = new_settings->gateway;
    memcpy(device->location, new_settings->location, TNP_MAX_LOCATION_LEN);
    ipv4_to_string(device->address, device->display_ip, sizeof(device->display_ip));
    snprintf(device->display_location, sizeof(device->display_location), "%s", device->location);
    snprintf(device->display_name, sizeof(device->display_name), "Reboot, please wait...");

    memset(&event, 0, sizeof(event));
    event.type = TNP_EVENT_DEVICE_REBOOT_PENDING;
    event.index = device_index;
    event.device = *device;
    emit_event(&event);

    return 0;
}

void tnp_engine_poll(int timeout_ms)
{
    int index;

    if (!g_running) {
        return;
    }

    for (index = 0; index < g_iface_count; ++index) {
        const int wait_ms = (index == 0) ? timeout_ms : 0;

        if (tnp_platform_poll_readable(g_ifaces[index].socket, wait_ms) > 0) {
            read_interface(index);
        }
    }
}

size_t tnp_engine_get_device_count(void)
{
    return g_device_count;
}

const tnp_device_t* tnp_engine_get_device(size_t index)
{
    if (index >= g_device_count) {
        return NULL;
    }

    return &g_devices[index];
}
