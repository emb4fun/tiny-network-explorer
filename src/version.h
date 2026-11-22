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
#ifndef TNE_VERSION_H
#define TNE_VERSION_H

/*
 * Central version and metadata for Tiny Network Explorer.
 * Used by the GUI (About dialog, window title) and the Windows PE
 * version resource (Explorer > Properties > Details).
 */

/* Shown in the UI (window title, About dialog). */
#define TNE_FILE_VERSION_STR        "2.00"
#define TNE_PRODUCT_VERSION         TNE_FILE_VERSION_STR

/* Numeric PE version (major, minor, build, revision). */
#define TNE_FILE_VERSION            2,0,0,0

/* Windows Properties > Details (PE version resource). */
#define TNE_FILE_DESCRIPTION        "Tiny Network Explorer"
#define TNE_PRODUCT_NAME            "Tiny Network Explorer"
#define TNE_INTERNAL_NAME           "Tiny Network Explorer"
#define TNE_ORIGINAL_FILENAME       "tnexpl.exe"

#define TNE_COPYRIGHT_HOLDER        "2021-2026 Michael Fischer"

/* UTF-8 for About dialog (Linux/Windows GUI). */
#define TNE_LEGAL_COPYRIGHT         "Copyright \xC2\xA9 " TNE_COPYRIGHT_HOLDER

/*
 * Latin-1 escape for windres (version.rc). Do not use UTF-8 here: \xC2\xA9
 * is misread and shows as an extra character before (C) in Properties > Details.
 */
#define TNE_LEGAL_COPYRIGHT_RC      "Copyright \251 " TNE_COPYRIGHT_HOLDER

#endif /* TNE_VERSION_H */
