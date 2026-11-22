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
#ifndef TNP_PROTOCOL_H
#define TNP_PROTOCOL_H

#include <stdint.h>

#define TNP_HEADER_MAGIC_1 0x54504E54u
#define TNP_HEADER_MAGIC_2 0x4E54504Eu
#define TNP_HEADER_VERSION 1u

#define TNP_UDP_PORT 54321u

#define TNP_NAME_LEN 16u
#define TNP_LOCATION_LEN 16u
#define TNP_MDNS_NAME_LEN 32u

#define TNP_MAX_NAME_LEN (TNP_NAME_LEN + 1u)
#define TNP_MAX_LOCATION_LEN (TNP_LOCATION_LEN + 1u)
#define TNP_MAX_MDNS_NAME_LEN (TNP_MDNS_NAME_LEN + 1u)

enum {
    TNP_SETUP_REQUEST = 0,
    TNP_SETUP_RESPONSE,
    TNP_SETUP_SET,
    TNP_SETUP_RESPONSE_ES
};

#if defined(_MSC_VER)
#pragma pack(push, 1)
#endif

typedef struct tnp_setup {
    uint32_t magic1;
    uint32_t magic2;
    uint16_t size;
    uint16_t version;
    uint8_t reserve;
    uint8_t mode;
    uint8_t use_dhcp;
    uint8_t mac_address[6];
    uint32_t address;
    uint32_t mask;
    uint32_t gateway;
    uint32_t fw_version;
    char name[TNP_MAX_NAME_LEN];
    char location[TNP_MAX_LOCATION_LEN];
    char mdns_name[TNP_MAX_MDNS_NAME_LEN];
}
#if defined(__GNUC__) || defined(__clang__)
__attribute__((packed))
#endif
tnp_setup_t;

#if defined(_MSC_VER)
#pragma pack(pop)
#endif

#endif /* TNP_PROTOCOL_H */
