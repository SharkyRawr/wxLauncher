/*
Copyright (C) 2009-2010 wxLauncher Team

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef TCMANAGER_H
#define TCMANAGER_H

#include <wx/wx.h>
#include <wx/clntdata.h>
#include <wx/filename.h>

/** Selected TC has changed. */
DECLARE_EVENT_TYPE(EVT_TC_CHANGED, wxID_ANY);
/** Selected binary has changed. */
DECLARE_EVENT_TYPE(EVT_TC_BINARY_CHANGED, wxID_ANY);
/** Selected mod has changed. */
DECLARE_EVENT_TYPE(EVT_TC_SELECTED_MOD_CHANGED, wxID_ANY);

WX_DECLARE_LIST(wxEvtHandler, TCEventHandlers);

class TCManager: public wxEvtHandler {
public:
	TCManager();
	~TCManager();

	static void Initialize();
	static void DeInitialize();
	static bool IsInitialized();
	static TCManager* Get();
private:
	static TCManager *manager;

public:
	void CurrentProfileChanged(wxCommandEvent &event);
	// Events
public:
	static void RegisterTCChanged(wxEvtHandler *handler);
	static void UnRegisterTCChanged(wxEvtHandler *handler);
	static void RegisterTCBinaryChanged(wxEvtHandler *handler);
	static void UnRegisterTCBinaryChanged(wxEvtHandler *handler);
	static void RegisterTCSelectedModChanged(wxEvtHandler *handler);
	static void UnRegisterTCSelectedModChanged(wxEvtHandler *handler);
	static void GenerateTCChanged();
	static void GenerateTCBinaryChanged();
	static void GenerateTCSelectedModChanged();
private:
	static TCEventHandlers TCChangedHandlers, TCBinaryChangedHandlers, TCSelectedModChangedHandlers;
	DECLARE_EVENT_TABLE();
};
#endif