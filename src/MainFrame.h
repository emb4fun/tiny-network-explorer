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
#ifndef MAIN_FRAME_H
#define MAIN_FRAME_H

#include <wx/wx.h>
#include <wx/checkbox.h>
#include <wx/listctrl.h>
#include <wx/timer.h>

extern "C" {
#include "tnp_engine.h"
}

class MainFrame : public wxFrame {
public:
    enum Column {
        COL_MAC = 0,
        COL_IP,
        COL_NAME,
        COL_LOCATION,
        COL_COUNT
    };

    MainFrame();
    ~MainFrame() override;

    void HandleEngineEvent(const tnp_event_t* event);

private:
    wxListCtrl* listCtrl_;
    wxCheckBox* useTinyNamesCheck_;
    wxTimer pollTimer_;
    bool engineInitialized_;
    bool engineRunning_;

    void BuildUi();
    void InitializeEngine();
    void ShutdownEngine();
    void StartPolling();
    void QueueEngineEvent(const tnp_event_t* event);

    void OnFirstShow(wxShowEvent& event);
    void OnSearch(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);
    void OnPollTimer(wxTimerEvent& event);
    void OnListMouseDown(wxMouseEvent& event);
    void OnListMouseDoubleClick(wxMouseEvent& event);
    void OnListItemActivated(wxListEvent& event);
    void OnColumnClick(wxListEvent& event);

    int ColumnAtPoint(const wxPoint& position) const;
    void HandleListDoubleClick(long row, int column);

    void SetupListColumns();
    void ResizeListColumns();
    void ApplyLegacyListStyle();
    void UpdateListCellPrefix();
    wxString FormatListCellText(const wxString& text) const;
    void UpdateListRow(int index);
    void ClearList();
    void OpenBrowserForRow(long row);
    void OpenChangeDialogForRow(long row);
    void AppendSystemMenuAbout();
    void CreateHelpMenu();

    static void EngineCallback(const tnp_event_t* event, void* userData);
    static int wxCALLBACK wxListCompare(wxIntPtr item1, wxIntPtr item2, wxIntPtr sortData);

    int sortColumn_;
    bool sortAscending_;
    int lastClickColumn_;
    wxString listCellPrefix_;

    wxDECLARE_EVENT_TABLE();

#ifdef __WXMSW__
    virtual WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam) override;
#endif
};

#endif /* MAIN_FRAME_H */
