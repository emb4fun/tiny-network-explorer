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
#include "AppIcon.h"

#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/toplevel.h>

#ifdef __WXMSW__
#include "resource.h"
#endif

wxString AppResourcePath(const wxString& fileName)
{
    const wxFileName executable(wxStandardPaths::Get().GetExecutablePath());
    return executable.GetPath() + wxFILE_SEP_PATH + wxT("resources") + wxFILE_SEP_PATH + fileName;
}

wxIconBundle LoadAppIconBundle()
{
    wxIconBundle icons;
    wxLogNull suppressLoadErrors;

#ifdef __WXMSW__
    wxIcon icon;
    icon.LoadFile(TNE_RC_APP_ICON, wxBITMAP_TYPE_ICO_RESOURCE);
    if (icon.IsOk()) {
        icons.AddIcon(icon);
    }
#else
    static const int kIconSizes[] = {16, 32, 48, 128, 256};

    for (int size : kIconSizes) {
        const wxString sizedPath = AppResourcePath(wxString::Format(wxT("app-%d.png"), size));
        wxIcon icon;

        if (wxFileExists(sizedPath) && icon.LoadFile(sizedPath, wxBITMAP_TYPE_PNG) && icon.IsOk()) {
            icons.AddIcon(icon);
        }
    }

    if (icons.IsEmpty()) {
        wxIcon icon;
        if (icon.LoadFile(AppResourcePath(wxT("app.png")), wxBITMAP_TYPE_PNG) && icon.IsOk()) {
            icons.AddIcon(icon);
        }
    }

    if (icons.IsEmpty()) {
        wxIcon icon;
        if (icon.LoadFile(AppResourcePath(wxT("app.ico")), wxBITMAP_TYPE_ICO) && icon.IsOk()) {
            icons.AddIcon(icon);
        }
    }
#endif

    return icons;
}

void ApplyWindowIcon(wxTopLevelWindow* window)
{
    if (window == nullptr) {
        return;
    }

    const wxIconBundle icons = LoadAppIconBundle();
    if (icons.IsEmpty()) {
        return;
    }

    window->SetIcons(icons);
}
