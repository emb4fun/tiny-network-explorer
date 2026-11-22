#pragma once

#include <wx/window.h>

inline wxFont LegacyGuiFont(wxWindow* window)
{
#ifdef __WXMSW__
    (void)window;
    return wxFont(
        8,
        wxFONTFAMILY_SWISS,
        wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL,
        false,
        wxT("MS Sans Serif"));
#else
    (void)window;
    return wxFont(
        8,
        wxFONTFAMILY_SWISS,
        wxFONTSTYLE_NORMAL,
        wxFONTWEIGHT_NORMAL,
        false);
#endif
}

inline wxSize LegacyDialogToPixels(wxWindow* window, const wxSize& dialogUnits)
{
    return window->ConvertDialogToPixels(dialogUnits);
}
