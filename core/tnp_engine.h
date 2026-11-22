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
#ifndef TNP_ENGINE_H
#define TNP_ENGINE_H

#include <stddef.h>

#include "tnp_device.h"

typedef enum tnp_event_type {
    TNP_EVENT_LIST_CLEARED = 0,
    TNP_EVENT_DEVICE_ADDED,
    TNP_EVENT_DEVICE_UPDATED,
    TNP_EVENT_DEVICE_REBOOT_PENDING,
    TNP_EVENT_ERROR
} tnp_event_type_t;

typedef struct tnp_event {
    tnp_event_type_t type;
    int index;
    tnp_device_t device;
    const char* message;
} tnp_event_t;

typedef void (*tnp_event_callback)(const tnp_event_t* event, void* user_data);

int tnp_engine_init(tnp_event_callback callback, void* user_data);
void tnp_engine_shutdown(void);

int tnp_engine_is_running(void);

int tnp_engine_search(void);

int tnp_engine_update(int device_index, const tnp_device_t* new_settings);

void tnp_engine_poll(int timeout_ms);

size_t tnp_engine_get_device_count(void);

const tnp_device_t* tnp_engine_get_device(size_t index);

#endif /* TNP_ENGINE_H */
