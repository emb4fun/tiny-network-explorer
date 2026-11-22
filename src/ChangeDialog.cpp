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
#include "ChangeDialog.h"

#include <cstring>

#include <wx/sizer.h>
#include "LegacyUiScale.h"
#include <wx/stattext.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

namespace {

wxString Ipv4ToString(uint32_t addressNetwork)
{
    char buffer[INET_ADDRSTRLEN] = {0};
    struct in_addr address;
    address.s_addr = addressNetwork;

    if (inet_ntop(AF_INET, &address, buffer, sizeof(buffer)) == nullptr) {
        return wxEmptyString;
    }

    return wxString::FromUTF8(buffer);
}

} /* namespace */

wxBEGIN_EVENT_TABLE(ChangeDialog, wxDialog)
    EVT_CHECKBOX(wxID_ANY, ChangeDialog::OnDhcpCheck)
    EVT_BUTTON(wxID_OK, ChangeDialog::OnOk)
    EVT_BUTTON(wxID_APPLY, ChangeDialog::OnDefault)
    EVT_BUTTON(wxID_CANCEL, ChangeDialog::OnCancel)
wxEND_EVENT_TABLE()

ChangeDialog::ChangeDialog(wxWindow* parent, const tnp_device_t& device, int deviceIndex)
    : wxDialog(
          parent,
          wxID_ANY,
          wxEmptyString,
          wxDefaultPosition,
          wxSize(173, 127),
          wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
    , device_(device)
    , deviceIndex_(deviceIndex)
    , hasChanges_(false)
    , nameLabel_(nullptr)
    , dhcpCheck_(nullptr)
    , ipCtrl_(nullptr)
    , maskCtrl_(nullptr)
    , gatewayCtrl_(nullptr)
    , locationCtrl_(nullptr)
    , okButton_(nullptr)
    , defaultButton_(nullptr)
{
    SetFont(LegacyGuiFont(this));

    SetTitle(wxString::Format(
        wxT("Change setting of %s"),
        wxString::FromUTF8(device_.display_mac)));

    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    nameLabel_ = new wxStaticText(
        this,
        wxID_ANY,
        wxString::Format(wxT("Name: %s"), wxString::FromUTF8(device_.name)));

    dhcpCheck_ = new wxCheckBox(this, wxID_ANY, wxT("Use DHCP"));
    dhcpCheck_->SetValue(device_.use_dhcp != 0);

    auto* gridSizer = new wxFlexGridSizer(4, 2, 4, 8);
    gridSizer->AddGrowableCol(1);

    gridSizer->Add(new wxStaticText(this, wxID_ANY, wxT("IP-Address :")), 0, wxALIGN_CENTER_VERTICAL);
    ipCtrl_ = new wxTextCtrl(this, wxID_ANY, Ipv4ToString(device_.address));
    gridSizer->Add(ipCtrl_, 1, wxEXPAND);

    gridSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Netmask :")), 0, wxALIGN_CENTER_VERTICAL);
    maskCtrl_ = new wxTextCtrl(this, wxID_ANY, Ipv4ToString(device_.mask));
    gridSizer->Add(maskCtrl_, 1, wxEXPAND);

    gridSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Gateway :")), 0, wxALIGN_CENTER_VERTICAL);
    gatewayCtrl_ = new wxTextCtrl(this, wxID_ANY, Ipv4ToString(device_.gateway));
    gridSizer->Add(gatewayCtrl_, 1, wxEXPAND);

    gridSizer->Add(new wxStaticText(this, wxID_ANY, wxT("Location:")), 0, wxALIGN_CENTER_VERTICAL);
    locationCtrl_ = new wxTextCtrl(this, wxID_ANY, wxString::FromUTF8(device_.location));
    locationCtrl_->SetMaxLength(TNP_LOCATION_LEN);
    gridSizer->Add(locationCtrl_, 1, wxEXPAND);

    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    okButton_ = new wxButton(this, wxID_OK, wxT("OK"));
    defaultButton_ = new wxButton(this, wxID_APPLY, wxT("Default"));
    auto* cancelButton = new wxButton(this, wxID_CANCEL, wxT("Cancel"));

    buttonSizer->Add(okButton_, 0, wxRIGHT, 6);
    buttonSizer->Add(defaultButton_, 0, wxRIGHT, 6);
    buttonSizer->Add(cancelButton, 0);

    rootSizer->Add(nameLabel_, 0, wxALL, 8);
    rootSizer->Add(dhcpCheck_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    rootSizer->Add(gridSizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 8);
    rootSizer->Add(buttonSizer, 0, wxALL | wxALIGN_RIGHT, 8);

    SetSizerAndFit(rootSizer);
    CentreOnParent();

    UpdateNetworkFieldsEnabled();

    if (device_.is_es == 0) {
        dhcpCheck_->Disable();
        ipCtrl_->Disable();
        maskCtrl_->Disable();
        gatewayCtrl_->Disable();
        locationCtrl_->Disable();
        okButton_->Disable();
        defaultButton_->Disable();
    }
}

void ChangeDialog::UpdateNetworkFieldsEnabled()
{
    const bool useDhcp = dhcpCheck_->IsChecked();
    ipCtrl_->Enable(!useDhcp);
    maskCtrl_->Enable(!useDhcp);
    gatewayCtrl_->Enable(!useDhcp);
}

void ChangeDialog::OnDhcpCheck(wxCommandEvent& event)
{
    (void)event;
    UpdateNetworkFieldsEnabled();
}

void ChangeDialog::OnDefault(wxCommandEvent& event)
{
    (void)event;

    ipCtrl_->SetValue(wxT("192.168.1.200"));
    maskCtrl_->SetValue(wxT("255.255.255.0"));
    gatewayCtrl_->SetValue(wxT("0.0.0.0"));
    locationCtrl_->SetValue(wxT("***"));
    dhcpCheck_->SetValue(true);
    UpdateNetworkFieldsEnabled();
}

bool ChangeDialog::ParseIpv4(const wxString& text, uint32_t* addressNetwork) const
{
    struct in_addr address;

    if (addressNetwork == nullptr) {
        return false;
    }

    const wxCharBuffer utf8 = text.ToUTF8();
    if (inet_pton(AF_INET, utf8.data(), &address) != 1) {
        return false;
    }

    *addressNetwork = address.s_addr;
    return true;
}

bool ChangeDialog::ApplySettingsFromControls()
{
    tnp_device_t updated = device_;
    uint32_t address = 0;
    uint32_t mask = 0;
    uint32_t gateway = 0;

    updated.use_dhcp = dhcpCheck_->IsChecked() ? 1 : 0;

    if (!ParseIpv4(ipCtrl_->GetValue(), &address) ||
        !ParseIpv4(maskCtrl_->GetValue(), &mask) ||
        !ParseIpv4(gatewayCtrl_->GetValue(), &gateway)) {
        wxMessageBox(
            wxT("Please enter valid IP addresses."),
            wxT("Tiny Network Explorer"),
            wxOK | wxICON_ERROR);
        return false;
    }

    updated.address = address;
    updated.mask = mask;
    updated.gateway = gateway;

    const wxString location = locationCtrl_->GetValue();
    std::strncpy(updated.location, location.utf8_str().data(), TNP_MAX_LOCATION_LEN - 1);
    updated.location[TNP_MAX_LOCATION_LEN - 1] = '\0';
    std::strncpy(updated.display_location, updated.location, sizeof(updated.display_location) - 1);
    updated.display_location[sizeof(updated.display_location) - 1] = '\0';

    if (updated.use_dhcp == device_.use_dhcp &&
        updated.address == device_.address &&
        updated.mask == device_.mask &&
        updated.gateway == device_.gateway &&
        std::strncmp(updated.location, device_.location, TNP_MAX_LOCATION_LEN) == 0) {
        return true;
    }

    if (tnp_engine_update(deviceIndex_, &updated) != 0) {
        wxMessageBox(
            wxT("Failed to send configuration update."),
            wxT("Tiny Network Explorer"),
            wxOK | wxICON_ERROR);
        return false;
    }

    hasChanges_ = true;
    return true;
}

void ChangeDialog::OnOk(wxCommandEvent& event)
{
    (void)event;

    if (ApplySettingsFromControls()) {
        EndModal(wxID_OK);
    }
}

void ChangeDialog::OnCancel(wxCommandEvent& event)
{
    (void)event;
    EndModal(wxID_CANCEL);
}
