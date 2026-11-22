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
#include "MainFrame.h"

#include <wx/dcclient.h>
#include <wx/image.h>
#include <wx/sizer.h>

#ifdef __WXMSW__
#include <commctrl.h>
#include <uxtheme.h>
#include <windows.h>
#endif

#include "AboutDialog.h"
#include "AppIcon.h"
#include "ChangeDialog.h"
#include "LegacyUiScale.h"
#include "version.h"

namespace {

enum {
    ID_SEARCH = wxID_HIGHEST + 1,
    ID_USE_TINYNAMES,
    ID_ABOUT_SYSMENU,
    ID_POLL_TIMER
};

constexpr int kMainDialogWidth = 370;
constexpr int kMainDialogHeight = 142;

wxString ColumnTitle(MainFrame::Column column)
{
    switch (column) {
    case MainFrame::COL_MAC:
        return wxT("MAC-Address");
    case MainFrame::COL_IP:
        return wxT("IP-Address");
    case MainFrame::COL_NAME:
        return wxT("Name");
    case MainFrame::COL_LOCATION:
        return wxT("Location");
    default:
        return wxEmptyString;
    }
}

} /* namespace */

wxBEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_SHOW(MainFrame::OnFirstShow)
    EVT_TIMER(ID_POLL_TIMER, MainFrame::OnPollTimer)
    EVT_BUTTON(wxID_EXIT, MainFrame::OnQuit)
    EVT_MENU(wxID_ABOUT, MainFrame::OnAbout)
wxEND_EVENT_TABLE()

void MainFrame::EngineCallback(const tnp_event_t* event, void* userData)
{
    auto* frame = static_cast<MainFrame*>(userData);
    if (frame == nullptr || event == nullptr) {
        return;
    }

    frame->QueueEngineEvent(event);
}

void MainFrame::QueueEngineEvent(const tnp_event_t* event)
{
    const tnp_event_t copy = *event;
    wxString errorMessage;

    if (event->type == TNP_EVENT_ERROR && event->message != nullptr) {
        errorMessage = wxString::FromUTF8(event->message);
    }

    CallAfter([this, copy, errorMessage]() {
        if (copy.type == TNP_EVENT_ERROR) {
            if (!errorMessage.empty()) {
                wxMessageBox(
                    errorMessage,
                    wxT("Tiny Network Explorer"),
                    wxOK | wxICON_ERROR);
            }
            return;
        }

        HandleEngineEvent(&copy);
    });
}

int wxCALLBACK MainFrame::wxListCompare(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData)
{
    auto* frame = reinterpret_cast<MainFrame*>(sortData);
    if (frame == nullptr) {
        return 0;
    }

    const wxString left = frame->listCtrl_->GetItemText(static_cast<long>(item1), frame->sortColumn_);
    const wxString right = frame->listCtrl_->GetItemText(static_cast<long>(item2), frame->sortColumn_);
    const int result = left.CmpNoCase(right);

    return frame->sortAscending_ ? result : -result;
}

MainFrame::MainFrame()
    : wxFrame(
          nullptr,
          wxID_ANY,
          wxString::Format(wxT("Tiny Network Explorer - v%s"), wxT(TNE_PRODUCT_VERSION)),
          wxDefaultPosition,
          wxDefaultSize,
          wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxSYSTEM_MENU)
    , listCtrl_(nullptr)
    , useTinyNamesCheck_(nullptr)
    , pollTimer_(this, ID_POLL_TIMER)
    , engineInitialized_(false)
    , engineRunning_(false)
    , sortColumn_(COL_MAC)
    , sortAscending_(true)
    , lastClickColumn_(COL_MAC)
{
    SetFont(LegacyGuiFont(this));

    BuildUi();

    const wxSize clientSize = LegacyDialogToPixels(this, wxSize(kMainDialogWidth, kMainDialogHeight));
    SetClientSize(clientSize);
    Layout();
    ResizeListColumns();

    const wxSize windowSize = GetSize();
    SetMinSize(windowSize);
    SetMaxSize(windowSize);
    Centre();
}

MainFrame::~MainFrame()
{
    ShutdownEngine();
}

void MainFrame::BuildUi()
{
    auto* panel = new wxPanel(this);
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    listCtrl_ = new wxListCtrl(
        panel,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES | wxBORDER_SUNKEN);
#ifdef __WXMSW__
    listCtrl_->EnableSystemTheme(false);
#endif

    SetupListColumns();
    ApplyLegacyListStyle();
    listCtrl_->Bind(wxEVT_LEFT_DOWN, &MainFrame::OnListMouseDown, this);
    listCtrl_->Bind(wxEVT_LEFT_DCLICK, &MainFrame::OnListMouseDoubleClick, this);
    listCtrl_->Bind(wxEVT_LIST_ITEM_ACTIVATED, &MainFrame::OnListItemActivated, this);
    listCtrl_->Bind(wxEVT_LIST_COL_CLICK, &MainFrame::OnColumnClick, this);

    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    auto* searchButton = new wxButton(panel, ID_SEARCH, wxT("Search"));
    useTinyNamesCheck_ = new wxCheckBox(panel, ID_USE_TINYNAMES, wxT("Use TINYnames"));
    auto* quitButton = new wxButton(panel, wxID_EXIT, wxT("Quit"));

    useTinyNamesCheck_->SetValue(true);

    searchButton->Bind(wxEVT_BUTTON, &MainFrame::OnSearch, this);

    buttonSizer->Add(searchButton, 0, wxRIGHT, 8);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(useTinyNamesCheck_, 0, wxALIGN_CENTER_VERTICAL);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(quitButton, 0);

    rootSizer->Add(listCtrl_, 1, wxEXPAND | wxALL, 10);
    rootSizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    panel->SetSizer(rootSizer);

    ApplyWindowIcon(this);

    CreateHelpMenu();
    AppendSystemMenuAbout();
}

void MainFrame::OnFirstShow(wxShowEvent& event)
{
    event.Skip();

    Layout();
    ResizeListColumns();
    UpdateListCellPrefix();

    if (engineInitialized_ || !event.IsShown()) {
        return;
    }

    engineInitialized_ = true;
    CallAfter([this]() {
        InitializeEngine();
    });
}

void MainFrame::InitializeEngine()
{
    if (tnp_engine_init(EngineCallback, this) != 0) {
        engineRunning_ = false;
        return;
    }

    engineRunning_ = true;
    StartPolling();

    if (tnp_engine_search() != 0) {
        wxMessageBox(
            wxT("Failed to send discovery broadcast."),
            wxT("Tiny Network Explorer"),
            wxOK | wxICON_ERROR);
    }
}

void MainFrame::StartPolling()
{
    pollTimer_.Start(100);
}

void MainFrame::ShutdownEngine()
{
    pollTimer_.Stop();

    if (engineRunning_) {
        tnp_engine_shutdown();
        engineRunning_ = false;
    }
}

void MainFrame::SetupListColumns()
{
    for (int column = 0; column < COL_COUNT; ++column) {
        listCtrl_->InsertColumn(column, ColumnTitle(static_cast<Column>(column)));
    }
}

void MainFrame::ResizeListColumns()
{
    int listWidth = listCtrl_->GetClientSize().GetWidth();
    if (listWidth <= 0) {
        const wxSize clientSize = GetClientSize();
        listWidth = clientSize.GetWidth() - 20;
    }
    if (listWidth <= 0) {
        return;
    }

    const int delta = listWidth / 26;
    const int macWidth = 6 * delta;
    const int ipWidth = 5 * delta;
    const int nameWidth = 9 * delta;
    const int locationWidth = listWidth - macWidth - ipWidth - nameWidth;

    listCtrl_->SetColumnWidth(COL_MAC, macWidth);
    listCtrl_->SetColumnWidth(COL_IP, ipWidth);
    listCtrl_->SetColumnWidth(COL_NAME, nameWidth);
    listCtrl_->SetColumnWidth(COL_LOCATION, locationWidth);
}

int MainFrame::ColumnAtPoint(const wxPoint& position) const
{
    const int scrollX = listCtrl_->GetScrollPos(wxHORIZONTAL);
    int columnLeft = -scrollX;

    for (int column = 0; column < COL_COUNT; ++column) {
        const int columnWidth = listCtrl_->GetColumnWidth(column);
        if (position.x >= columnLeft && position.x < columnLeft + columnWidth) {
            return column;
        }
        columnLeft += columnWidth;
    }

    return -1;
}

void MainFrame::HandleListDoubleClick(long row, int column)
{
    if (column == COL_MAC) {
        OpenChangeDialogForRow(row);
    } else if (column == COL_IP) {
        OpenBrowserForRow(row);
    }
}

void MainFrame::ApplyLegacyListStyle()
{
#ifdef __WXMSW__
    const HWND listView = reinterpret_cast<HWND>(listCtrl_->GetHandle());
    if (listView == nullptr) {
        return;
    }

    ListView_SetExtendedListViewStyle(
        listView,
        ListView_GetExtendedListViewStyle(listView) | LVS_EX_GRIDLINES);

    const HWND header = ListView_GetHeader(listView);
    if (header != nullptr) {
        ::SetWindowTheme(header, L"", L"");
    }
#endif
}

void MainFrame::UpdateListCellPrefix()
{
#ifndef __WXMSW__
    const int targetPx = LegacyDialogToPixels(listCtrl_, wxSize(2, 0)).GetWidth();
    if (targetPx <= 0) {
        listCellPrefix_ = wxT(" ");
        return;
    }

    wxClientDC dc(listCtrl_);
    dc.SetFont(listCtrl_->GetFont());

    listCellPrefix_.clear();
    int width = 0;
    while (width < targetPx) {
        listCellPrefix_ += wxT(" ");
        dc.GetTextExtent(listCellPrefix_, &width, nullptr);
    }

    for (int column = 0; column < COL_COUNT; ++column) {
        wxListItem header;
        header.SetMask(wxLIST_MASK_TEXT);
        header.SetText(FormatListCellText(ColumnTitle(static_cast<Column>(column))));
        listCtrl_->SetColumn(column, header);
    }
#else
    listCellPrefix_.clear();
#endif
}

wxString MainFrame::FormatListCellText(const wxString& text) const
{
    return listCellPrefix_ + text;
}

void MainFrame::HandleEngineEvent(const tnp_event_t* event)
{
    if (event == nullptr) {
        return;
    }

    switch (event->type) {
    case TNP_EVENT_LIST_CLEARED:
        ClearList();
        break;

    case TNP_EVENT_DEVICE_ADDED:
    case TNP_EVENT_DEVICE_UPDATED:
    case TNP_EVENT_DEVICE_REBOOT_PENDING:
        UpdateListRow(event->index);
        break;

    case TNP_EVENT_ERROR:
        if (event->message != nullptr) {
            wxMessageBox(
                wxString::FromUTF8(event->message),
                wxT("Tiny Network Explorer"),
                wxOK | wxICON_ERROR);
        }
        break;

    default:
        break;
    }
}

void MainFrame::ClearList()
{
    listCtrl_->DeleteAllItems();
}

void MainFrame::UpdateListRow(int index)
{
    const tnp_device_t* device = tnp_engine_get_device(static_cast<size_t>(index));
    if (device == nullptr) {
        return;
    }

    long row = wxNOT_FOUND;
    for (long item = 0; item < listCtrl_->GetItemCount(); ++item) {
        if (listCtrl_->GetItemData(item) == static_cast<long>(index)) {
            row = item;
            break;
        }
    }

    if (row == wxNOT_FOUND) {
        row = listCtrl_->InsertItem(
            listCtrl_->GetItemCount(),
            FormatListCellText(wxString::FromUTF8(device->display_mac)));
        listCtrl_->SetItemData(row, static_cast<long>(index));
    }

    listCtrl_->SetItem(row, COL_MAC, FormatListCellText(wxString::FromUTF8(device->display_mac)));
    listCtrl_->SetItem(row, COL_IP, FormatListCellText(wxString::FromUTF8(device->display_ip)));
    listCtrl_->SetItem(row, COL_NAME, FormatListCellText(wxString::FromUTF8(device->display_name)));
    listCtrl_->SetItem(row, COL_LOCATION, FormatListCellText(wxString::FromUTF8(device->display_location)));
}

void MainFrame::OnSearch(wxCommandEvent& event)
{
    (void)event;

    if (!engineRunning_) {
        wxMessageBox(
            wxT("Network engine is not running."),
            wxT("Tiny Network Explorer"),
            wxOK | wxICON_ERROR);
        return;
    }

    if (tnp_engine_search() != 0) {
        wxMessageBox(
            wxT("Failed to send discovery broadcast."),
            wxT("Tiny Network Explorer"),
            wxOK | wxICON_ERROR);
    }
}

void MainFrame::OnQuit(wxCommandEvent& event)
{
    (void)event;
    Close(true);
}

void MainFrame::OnAbout(wxCommandEvent& event)
{
    (void)event;
    AboutDialog dialog(this);
    dialog.ShowModal();
}

#ifdef __WXMSW__
WXLRESULT MainFrame::MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam)
{
    if (message == WM_SYSCOMMAND && wParam == static_cast<WXWPARAM>(ID_ABOUT_SYSMENU)) {
        AboutDialog dialog(this);
        dialog.ShowModal();
        return 0;
    }

    return wxFrame::MSWWindowProc(message, wParam, lParam);
}
#endif

void MainFrame::OnPollTimer(wxTimerEvent& event)
{
    (void)event;

    if (!engineRunning_) {
        return;
    }

    tnp_engine_poll(50);
}

void MainFrame::OnListMouseDown(wxMouseEvent& event)
{
    const int column = ColumnAtPoint(event.GetPosition());
    if (column >= 0) {
        lastClickColumn_ = column;
    }

    event.Skip();
}

void MainFrame::OnListMouseDoubleClick(wxMouseEvent& event)
{
    const wxPoint position = event.GetPosition();
    int flags = 0;
    const long row = listCtrl_->HitTest(position, flags);

    if (row == wxNOT_FOUND) {
        event.Skip();
        return;
    }

    const int column = ColumnAtPoint(position);
    if (column >= 0) {
        lastClickColumn_ = column;
    }

#ifdef __WXMSW__
    HandleListDoubleClick(row, column >= 0 ? column : lastClickColumn_);
#endif
    event.Skip();
}

void MainFrame::OnListItemActivated(wxListEvent& event)
{
#ifndef __WXMSW__
    HandleListDoubleClick(event.GetIndex(), lastClickColumn_);
#endif
    event.Skip();
}

void MainFrame::OpenBrowserForRow(long row)
{
    const long deviceIndex = listCtrl_->GetItemData(row);
    const tnp_device_t* device = tnp_engine_get_device(static_cast<size_t>(deviceIndex));
    if (device == nullptr) {
        return;
    }

    wxString url;
    const bool useTinyNames = useTinyNamesCheck_ != nullptr && useTinyNamesCheck_->IsChecked();
    if (useTinyNames && device->mdns_name[0] != '\0') {
        url = wxString::Format(wxT("http://%s"), wxString::FromUTF8(device->mdns_name));
    } else {
        url = wxString::Format(wxT("http://%s"), wxString::FromUTF8(device->display_ip));
    }

    if (!wxLaunchDefaultBrowser(url)) {
        wxMessageBox(
            wxString::Format(wxT("Could not open browser for %s"), url),
            wxT("Tiny Network Explorer"),
            wxOK | wxICON_ERROR);
    }
}

void MainFrame::OpenChangeDialogForRow(long row)
{
    const long deviceIndex = listCtrl_->GetItemData(row);
    const tnp_device_t* device = tnp_engine_get_device(static_cast<size_t>(deviceIndex));
    if (device == nullptr) {
        return;
    }

    ChangeDialog dialog(this, *device, static_cast<int>(deviceIndex));
    dialog.ShowModal();
}

void MainFrame::OnColumnClick(wxListEvent& event)
{
    const int column = event.GetColumn();
    if (column < 0 || column >= COL_COUNT) {
        return;
    }

    if (sortColumn_ == column) {
        sortAscending_ = !sortAscending_;
    } else {
        sortColumn_ = column;
        sortAscending_ = true;
    }

    listCtrl_->SortItems(wxListCompare, reinterpret_cast<wxIntPtr>(this));
}

void MainFrame::CreateHelpMenu()
{
    auto* menuBar = new wxMenuBar();
    auto* helpMenu = new wxMenu();
    helpMenu->Append(wxID_ABOUT, wxT("&About Tiny Network Explorer..."));
    menuBar->Append(helpMenu, wxT("&Help"));
    SetMenuBar(menuBar);
}

void MainFrame::AppendSystemMenuAbout()
{
#ifdef __WXMSW__
    HMENU systemMenu = ::GetSystemMenu(GetHWND(), FALSE);
    if (systemMenu == nullptr) {
        return;
    }

    ::AppendMenuW(systemMenu, MF_SEPARATOR, 0, nullptr);
    ::AppendMenuW(systemMenu, MF_STRING, ID_ABOUT_SYSMENU, L"About");
#endif
}
