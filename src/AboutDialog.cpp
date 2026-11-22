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
#include "AboutDialog.h"

#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

#ifdef __WXMSW__
#include <wx/msw/wrapwin.h>
#endif

#include "AppIcon.h"
#include "LegacyUiScale.h"
#include "resource.h"
#include "version.h"

namespace {

bool LoadEmbeddedBitmap(wxBitmap& bitmap, const wxChar* resourceName, const wxString& fileName)
{
#ifdef __WXMSW__
    (void)fileName;
    return bitmap.LoadFile(resourceName, wxBITMAP_TYPE_BMP_RESOURCE);
#else
    (void)resourceName;
    bitmap = wxBitmap(AppResourcePath(fileName), wxBITMAP_TYPE_BMP);
    return bitmap.IsOk();
#endif
}

} /* namespace */

AboutDialog::AboutDialog(wxWindow* parent)
    : wxDialog(
          parent,
          wxID_ANY,
          wxT("About"),
          wxDefaultPosition,
          wxSize(157, 71),
          wxDEFAULT_DIALOG_STYLE)
{
    SetFont(LegacyGuiFont(this));

    auto* rootSizer = new wxBoxSizer(wxHORIZONTAL);

    wxColour background = GetBackgroundColour();
    if (!background.IsOk()) {
        background = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
    }

    auto* bitmap = new wxStaticBitmap(this, wxID_ANY, LoadAboutChipBitmap(background));
    auto* textSizer = new wxBoxSizer(wxVERTICAL);
    auto* versionLabel = new wxStaticText(
        this,
        wxID_ANY,
        wxString::Format(wxT("Tiny Network Explorer - v%s"), wxT(TNE_PRODUCT_VERSION)));
    auto* copyrightLabel = new wxStaticText(
        this,
        wxID_ANY,
        wxString::FromUTF8(TNE_LEGAL_COPYRIGHT));

    textSizer->Add(versionLabel, 0, wxBOTTOM, 4);
    textSizer->Add(copyrightLabel, 0);

    rootSizer->Add(bitmap, 0, wxALL, 8);
    rootSizer->Add(textSizer, 1, wxALL | wxALIGN_CENTER_VERTICAL, 8);

    auto* buttonSizer = new wxBoxSizer(wxVERTICAL);
    auto* okButton = new wxButton(this, wxID_OK, wxT("OK"));
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(okButton, 0, wxALIGN_CENTER_HORIZONTAL | wxBOTTOM, 8);

    auto* outerSizer = new wxBoxSizer(wxVERTICAL);
    outerSizer->Add(rootSizer, 1, wxEXPAND);
    outerSizer->Add(buttonSizer, 0, wxEXPAND);

    SetSizerAndFit(outerSizer);
    CentreOnParent();

    Bind(wxEVT_BUTTON, [this](wxCommandEvent& event) {
        if (event.GetId() == wxID_OK) {
            EndModal(wxID_OK);
        }
    });
}

wxBitmap AboutDialog::LoadAboutChipBitmap(const wxColour& background) const
{
    wxBitmap chip;
    wxBitmap shape;

    if (!LoadEmbeddedBitmap(chip, TNE_RC_CHIP, wxT("nutchip.bmp"))) {
        return wxBitmap();
    }

    if (!LoadEmbeddedBitmap(shape, TNE_RC_CHIP_SHAPE, wxT("nutchips.bmp"))) {
        return chip;
    }

    const int width = chip.GetWidth();
    const int height = chip.GetHeight();
    const wxColour fill = background.IsOk() ? background : wxColour(192, 192, 192);

#ifdef __WXMSW__
    wxBitmap composed(width, height, 24);
    wxMemoryDC destination;
    destination.SelectObject(composed);
    destination.SetBackground(wxBrush(fill));
    destination.Clear();

    wxMemoryDC shapeDc;
    wxMemoryDC chipDc;
    shapeDc.SelectObject(shape);
    chipDc.SelectObject(chip);

    HDC destHdc = static_cast<HDC>(destination.GetHDC());
    ::BitBlt(destHdc, 0, 0, width, height, static_cast<HDC>(shapeDc.GetHDC()), 0, 0, SRCPAINT);
    ::BitBlt(destHdc, 0, 0, width, height, static_cast<HDC>(chipDc.GetHDC()), 0, 0, SRCAND);

    destination.SelectObject(wxNullBitmap);

    wxImage image = composed.ConvertToImage();
    image.SetMask(false);
    return wxBitmap(image);
#else
    wxImage dest(width, height);
    wxImage shapeImage = shape.ConvertToImage();
    wxImage chipImage = chip.ConvertToImage();

    if (!dest.IsOk() || !shapeImage.IsOk() || !chipImage.IsOk()) {
        return chip;
    }

    unsigned char* destData = dest.GetData();
    const unsigned char* shapeData = shapeImage.GetData();
    const unsigned char* chipData = chipImage.GetData();
    const unsigned char fillRed = fill.Red();
    const unsigned char fillGreen = fill.Green();
    const unsigned char fillBlue = fill.Blue();

    for (int index = 0; index < width * height; ++index) {
        const int offset = index * 3;

        destData[offset + 0] = fillRed;
        destData[offset + 1] = fillGreen;
        destData[offset + 2] = fillBlue;

        // Same compositing as legacy GDI: SRCPAINT (shape), then SRCAND (chip).
        destData[offset + 0] = static_cast<unsigned char>(destData[offset + 0] | shapeData[offset + 0]);
        destData[offset + 1] = static_cast<unsigned char>(destData[offset + 1] | shapeData[offset + 1]);
        destData[offset + 2] = static_cast<unsigned char>(destData[offset + 2] | shapeData[offset + 2]);

        destData[offset + 0] = static_cast<unsigned char>(destData[offset + 0] & chipData[offset + 0]);
        destData[offset + 1] = static_cast<unsigned char>(destData[offset + 1] & chipData[offset + 1]);
        destData[offset + 2] = static_cast<unsigned char>(destData[offset + 2] & chipData[offset + 2]);
    }

    dest.SetMask(false);
    return wxBitmap(dest);
#endif
}
