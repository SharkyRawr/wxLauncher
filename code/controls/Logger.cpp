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

#include <wx/wx.h>
#include <wx/wfstream.h>
#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include <wchar.h>

#include "controls/Logger.h"
#include "controls/StatusBar.h"

#include "global/MemoryDebugging.h"

////// Logger
const wxString levels[] = {
	_T("FATAL"),
	_T("ERROR"),
	_T("WARN "),
	_T("MSG  "),
	_T("STSBR"),
	_T("INFO "),
	_T("DEBUG"),
};
/** Constructor. */
Logger::Logger() {
	wxFileName outfile(wxStandardPaths::Get().GetUserDataDir(), _T("wxLauncher.log"));
	if (!outfile.DirExists() && 
		!wxFileName::Mkdir(outfile.GetPath(), 0700, wxPATH_MKDIR_FULL) ) {
			wxLogFatalError(_T("Unable to create folder to place log in. (%s)"), outfile.GetPath().c_str());
	}
	this->out = new wxFFileOutputStream(outfile.GetFullPath(), _T("wb"));
	wxASSERT_MSG(out->IsOk(), _T("Log output file is not valid!"));
	this->out->Write("\357\273\277", 3);

	this->statusBar = NULL;
}

/** Destructor. */
Logger::~Logger() {
	char exitmsg[] = "\nLog closed.\n";
	this->out->Write(exitmsg, sizeof(exitmsg));
	this->out->Close();
	delete this->out;
}

/** Overridden as per wxWidgets docs to implement a wxLog. */
void Logger::DoLog(wxLogLevel level, const wxChar *msg, time_t time) {
	wxString timestr = wxDateTime(time).Format(_T("%y%j%H%M%S"), wxDateTime::GMT0);
	wxString str = wxString::Format(
    _T("%s:%s:"), timestr.c_str(), levels[level].c_str());
	wxString buf(msg);
	out->Write(str.mb_str(wxConvUTF8), str.size());
	out->Write(buf.mb_str(wxConvUTF8), buf.size());
	out->Write("\n", 1);

	if ( this->statusBar != NULL ) {
		if ( level == 1 ) { // error
			this->statusBar->SetMainStatusText(buf, ID_SB_ERROR);
		} else if ( level == 2 ) { // warning
			this->statusBar->SetMainStatusText(buf, ID_SB_WARNING);
		} else if ( level == 3 || level == 4 ) { // message, statubar
			this->statusBar->SetMainStatusText(buf, ID_SB_OK);
		} else if ( level == 5 ) { // info
			this->statusBar->SetMainStatusText(buf, ID_SB_INFO);
		}
	}		
}

/** Stores the pointer the status bar that I am to send status messages to.
If a status bar is already set, function will do nothing to the old statusbar.
Logger does not take over managment of the statusbar passed in. */
void Logger::SetStatusBarTarget(StatusBar *bar) {
	this->statusBar = bar;
}
