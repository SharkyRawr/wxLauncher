#include <wx/wx.h>
#include "ModsPage.h"
#include "wxIDs.h"
#include "ModGrid.h"

#include "wxLauncherSetup.h" // Last include for memory debugging

ModsPage::ModsPage(wxWindow* parent): wxPanel(parent, wxID_ANY) {
	wxStaticText* header = new wxStaticText(this, wxID_ANY,
		_("Installed MODs.  Click on Install/Update in the left panel to\n\
		  search, download, and install additional MODs and updates."));

	wxGrid* modGrid = new ModGrid(this, wxSize(600, 400));

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	sizer->Add(header);
	sizer->Add(modGrid);

	this->SetSizer(sizer);
	this->Layout();

}