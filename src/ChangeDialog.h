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
#ifndef CHANGE_DIALOG_H
#define CHANGE_DIALOG_H

#include <wx/wx.h>

extern "C" {
#include "tnp_engine.h"
}

class ChangeDialog : public wxDialog {
public:
    ChangeDialog(wxWindow* parent, const tnp_device_t& device, int deviceIndex);

    bool HasChanges() const { return hasChanges_; }

private:
    tnp_device_t device_;
    int deviceIndex_;
    bool hasChanges_;

    wxStaticText* nameLabel_;
    wxCheckBox* dhcpCheck_;
    wxTextCtrl* ipCtrl_;
    wxTextCtrl* maskCtrl_;
    wxTextCtrl* gatewayCtrl_;
    wxTextCtrl* locationCtrl_;
    wxButton* okButton_;
    wxButton* defaultButton_;

    void OnDhcpCheck(wxCommandEvent& event);
    void OnDefault(wxCommandEvent& event);
    void OnOk(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);
    void UpdateNetworkFieldsEnabled();

    bool ApplySettingsFromControls();
    bool ParseIpv4(const wxString& text, uint32_t* addressNetwork) const;

    wxDECLARE_EVENT_TABLE();
};

#endif /* CHANGE_DIALOG_H */
