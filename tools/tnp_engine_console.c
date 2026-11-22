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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "tnp_engine.h"
#include "tnp_protocol.h"

typedef struct app_options {
    int timeout_seconds;
    int quiet;
} app_options_t;

static void ipv4_to_string(uint32_t address_network, char* buffer, size_t buffer_length)
{
    const unsigned char* bytes = (const unsigned char*)&address_network;

    snprintf(
        buffer,
        buffer_length,
        "%u.%u.%u.%u",
        bytes[0],
        bytes[1],
        bytes[2],
        bytes[3]);
}

static void print_usage(const char* program_name)
{
    printf(
        "Tiny Network Explorer - console test tool\n\n"
        "Usage: %s [options]\n\n"
        "Options:\n"
        "  -t, --timeout SEC   Wait for device responses (default: 10)\n"
        "  -q, --quiet         Suppress live events, print summary only\n"
        "  -h, --help          Show this help\n\n"
        "Example:\n"
        "  %s -t 15\n",
        program_name,
        program_name);
}

static int parse_options(int argc, char** argv, app_options_t* options)
{
    int index;

    options->timeout_seconds = 10;
    options->quiet = 0;

    for (index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "-h") == 0 || strcmp(argv[index], "--help") == 0) {
            print_usage(argv[0]);
            return 1;
        }

        if (strcmp(argv[index], "-q") == 0 || strcmp(argv[index], "--quiet") == 0) {
            options->quiet = 1;
            continue;
        }

        if (strcmp(argv[index], "-t") == 0 || strcmp(argv[index], "--timeout") == 0) {
            if (index + 1 >= argc) {
                fprintf(stderr, "Missing value for %s\n", argv[index]);
                return -1;
            }

            options->timeout_seconds = atoi(argv[index + 1]);
            if (options->timeout_seconds <= 0) {
                fprintf(stderr, "Timeout must be greater than 0.\n");
                return -1;
            }

            ++index;
            continue;
        }

        fprintf(stderr, "Unknown option: %s\n", argv[index]);
        print_usage(argv[0]);
        return -1;
    }

    return 0;
}

static void print_device_details(size_t index, const tnp_device_t* device)
{
    char mask[40];
    char gateway[40];

    if (device == NULL) {
        return;
    }

    ipv4_to_string(device->mask, mask, sizeof(mask));
    ipv4_to_string(device->gateway, gateway, sizeof(gateway));

    printf("Device #%zu\n", index);
    printf("  MAC address : %s\n", device->display_mac);
    printf("  IP address  : %s\n", device->display_ip);
    printf("  Netmask     : %s\n", mask);
    printf("  Gateway     : %s\n", gateway);
    printf("  DHCP        : %s\n", device->use_dhcp ? "yes" : "no");
    printf("  Name        : %s\n", device->name);
    printf("  Location    : %s\n", device->location);
    printf("  mDNS name   : %s\n", device->mdns_name[0] != '\0' ? device->mdns_name : "-");
    printf("  ES device   : %s\n", device->is_es ? "yes" : "no");
    printf("\n");
}

static void print_device_summary(const tnp_device_t* device)
{
    if (device == NULL) {
        return;
    }

    printf(
        "  %s | %s | %s | %s\n",
        device->display_mac,
        device->display_ip,
        device->display_name,
        device->display_location);
}

static void on_event(const tnp_event_t* event, void* user_data)
{
    const app_options_t* options = (const app_options_t*)user_data;

    if (event == NULL || options == NULL) {
        return;
    }

    if (options->quiet) {
        return;
    }

    switch (event->type) {
    case TNP_EVENT_LIST_CLEARED:
        printf("Device list cleared.\n");
        break;

    case TNP_EVENT_DEVICE_ADDED:
        printf("Device added (index %d):\n", event->index);
        print_device_details((size_t)event->index, &event->device);
        break;

    case TNP_EVENT_DEVICE_UPDATED:
        printf("Device updated (index %d):\n", event->index);
        print_device_details((size_t)event->index, &event->device);
        break;

    case TNP_EVENT_DEVICE_REBOOT_PENDING:
        printf("Device reboot pending (index %d): %s\n", event->index, event->device.display_name);
        break;

    case TNP_EVENT_ERROR:
        fprintf(stderr, "Error: %s\n", event->message != NULL ? event->message : "unknown error");
        break;

    default:
        break;
    }
}

static void sleep_ms(int milliseconds)
{
#ifdef _WIN32
    Sleep((DWORD)milliseconds);
#else
    usleep((useconds_t)milliseconds * 1000);
#endif
}

static void print_final_summary(void)
{
    size_t count;
    size_t index;

    count = tnp_engine_get_device_count();
    printf("Summary: %zu device(s) found.\n\n", count);

    if (count == 0) {
        printf("No Tiny Server devices responded in this subnet.\n");
        return;
    }

    printf("  MAC-Address        | IP-Address     | Name                         | Location\n");
    printf("  -------------------+----------------+------------------------------+----------\n");

    for (index = 0; index < count; ++index) {
        print_device_summary(tnp_engine_get_device(index));
    }

    printf("\nDetailed device data:\n\n");
    for (index = 0; index < count; ++index) {
        print_device_details(index, tnp_engine_get_device(index));
    }
}

int main(int argc, char** argv)
{
    app_options_t options;
    int parse_result;
    int second;

    parse_result = parse_options(argc, argv, &options);
    if (parse_result != 0) {
        return parse_result > 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (tnp_engine_init(on_event, &options) != 0) {
        fprintf(stderr, "tnp_engine_init failed\n");
        return EXIT_FAILURE;
    }

    printf("Sending TNP discovery broadcast on UDP port %u...\n", (unsigned)TNP_UDP_PORT);
    printf("Listening for responses for %d second(s).\n\n", options.timeout_seconds);

    if (tnp_engine_search() != 0) {
        fprintf(stderr, "tnp_engine_search failed\n");
        tnp_engine_shutdown();
        return EXIT_FAILURE;
    }

    for (second = 0; second < options.timeout_seconds; ++second) {
        tnp_engine_poll(100);
        sleep_ms(900);
    }

    print_final_summary();
    tnp_engine_shutdown();
    return EXIT_SUCCESS;
}
