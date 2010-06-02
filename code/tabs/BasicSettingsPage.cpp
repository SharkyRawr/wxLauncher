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
#include <wx/filename.h>
#include <wx/choicebk.h>

#include "generated/configure_launcher.h"

#if HAS_SDL == 1
#include "SDL.h"
#endif

#include "tabs/BasicSettingsPage.h"
#include "global/ids.h"
#include "apis/ProfileManager.h"
#include "apis/TCManager.h"
#include "apis/SpeechManager.h"
#include "apis/OpenALManager.h"
#include "apis/JoystickManager.h"
#include "apis/HelpManager.h"
#include "datastructures/FSOExecutable.h"

#include "global/MemoryDebugging.h" // Last include for memory debugging

class ProxyChoice: public wxChoicebook {
public:
	ProxyChoice(wxWindow *parent, wxWindowID id);
	virtual ~ProxyChoice();

	void OnChangeServer(wxCommandEvent &event);
	void OnChangePort(wxCommandEvent &event);
	void OnProxyTypeChange(wxChoicebookEvent &event);

private:


	DECLARE_EVENT_TABLE();
};

class ExeChoice: public wxChoice {
public:
	ExeChoice(wxWindow * parent, wxWindowID id) : wxChoice(parent, id) {}
	bool FindAndSetSelectionWithClientData(wxString item) {
		size_t number = this->GetStrings().size();
		for( size_t i = 0; i < number; i++ ) {
			FSOExecutable* data = dynamic_cast<FSOExecutable*>(this->GetClientObject(i));
			wxCHECK2_MSG( data != NULL, continue, _T("Client data is not a FSOVersion pointer"));
			if ( data->GetExecutableName() == item ) {
				this->SetSelection(i);
				return true;
			}
		}
		return false;
	}
};
		

BasicSettingsPage::BasicSettingsPage(wxWindow* parent): wxPanel(parent, wxID_ANY) {
	TCManager::Initialize();
	ProMan::GetProfileManager()->AddEventHandler(this);
	wxCommandEvent event(this->GetId());
	this->ProfileChanged(event);
}

void BasicSettingsPage::ProfileChanged(wxCommandEvent &WXUNUSED(event)) {
	if (this->GetSizer() != NULL) {
		this->GetSizer()->DeleteWindows();
	}

	ProMan* proman = ProMan::GetProfileManager();
	// exe Selection
	wxString tcfolder, binary;
	bool hastcfolder = proman->Get()->Read(PRO_CFG_TC_ROOT_FOLDER, &tcfolder, _T(""));
	proman->Get()->Read(PRO_CFG_TC_CURRENT_BINARY, &binary, _T(""));
	
	wxStaticBox* exeBox = new wxStaticBox(this, wxID_ANY, _("TC root folder and Executable"));

	wxStaticText* rootFolderText = new wxStaticText(this, ID_EXE_ROOT_FOLDER_BOX_TEXT, _("FS2 Root Folder:"));
	wxTextCtrl* rootFolderBox = new wxTextCtrl(this, ID_EXE_ROOT_FOLDER_BOX, tcfolder);
	wxButton* selectButton = new wxButton(this, ID_EXE_SELECT_ROOT_BUTTON, _T("Select"));

	rootFolderBox->SetEditable(false);

	wxStaticText* useExeText = new wxStaticText(this, wxID_ANY, _("Use this FS2_Open binary: "));
	ExeChoice* useExeChoice = new ExeChoice(this, ID_EXE_CHOICE_BOX);
	if ( hastcfolder ) {
		this->FillExecutableDropBox(useExeChoice, wxFileName(tcfolder, wxEmptyString));
		useExeChoice->FindAndSetSelectionWithClientData(binary);
	} else {
		useExeChoice->Disable();
	}
	TCManager::RegisterTCChanged(this);

	wxBoxSizer* rootFolderSizer = new wxBoxSizer(wxHORIZONTAL);
	rootFolderSizer->Add(rootFolderText);
	rootFolderSizer->Add(rootFolderBox, wxSizerFlags().Proportion(1));
	rootFolderSizer->Add(selectButton);

	wxBoxSizer* selectExeSizer = new wxBoxSizer(wxHORIZONTAL);
	selectExeSizer->Add(useExeText);
	selectExeSizer->Add(useExeChoice, wxSizerFlags().Proportion(1));

	wxStaticBoxSizer* exeSizer = new wxStaticBoxSizer(exeBox, wxVERTICAL);
	exeSizer->Add(rootFolderSizer, wxSizerFlags().Expand());
	exeSizer->Add(selectExeSizer,  wxSizerFlags().Expand());

	// Video Section
	wxStaticBox* videoBox = new wxStaticBox(this, ID_VIDEO_STATIC_BOX, _("Video"));

	wxStaticText* graphicsText = 
		new wxStaticText(this, wxID_ANY, _("Graphics:"));
	wxChoice* graphicsCombo = new wxChoice(this, ID_GRAPHICS_COMBO);
	graphicsCombo->Insert(_T("OpenGL"), 0);
	wxString graphicsAPI;
	proman->Get()->Read(PRO_CFG_VIDEO_API, &graphicsAPI, _T("OpenGL"));
	graphicsCombo->SetStringSelection(graphicsAPI);
	graphicsCombo->Disable();

	wxStaticText* resolutionText = 
		new wxStaticText(this, wxID_ANY, _("Resolution:"));
	wxChoice* resolutionCombo = new wxChoice(this, ID_RESOLUTION_COMBO);
	this->FillResolutionDropBox(resolutionCombo);
	int width, height;
	proman->Get()->Read(PRO_CFG_VIDEO_RESOLUTION_WIDTH, &width, 800);
	proman->Get()->Read(PRO_CFG_VIDEO_RESOLUTION_HEIGHT, &height, 600);
	resolutionCombo->SetStringSelection(
		wxString::Format(CFG_RES_FORMAT_STRING, width, height));

	wxStaticText* depthText = 
		new wxStaticText(this, wxID_ANY, _("Depth:"));
	wxChoice* depthCombo = new wxChoice(this, ID_DEPTH_COMBO);
	int bitDepth;
	depthCombo->Append(_("16 bit"));
	depthCombo->Append(_("32 bit"));
	proman->Get()->Read(PRO_CFG_VIDEO_BIT_DEPTH, &bitDepth, 16);
	depthCombo->SetSelection((bitDepth == 16) ? 0 : 1);

	wxStaticText* textureFilterText = 
		new wxStaticText(this, wxID_ANY, _("Texture Filter:"));
	wxChoice* textureFilterCombo = new wxChoice(this, ID_TEXTURE_FILTER_COMBO);
	wxString filter;
	textureFilterCombo->Append(_("Bilinear"));
	textureFilterCombo->Append(_("Trilinear"));
	proman->Get()->Read(PRO_CFG_VIDEO_TEXTURE_FILTER, &filter, _T("bilinear"));
	filter.MakeLower();
	textureFilterCombo->SetSelection( (filter == _T("bilinear")) ? 0 : 1);

	wxStaticText* anisotropicText = 
		new wxStaticText(this, wxID_ANY, _("Anisotropic:"));
	wxChoice* anisotropicCombo = new wxChoice(this, ID_ANISOTROPIC_COMBO);
	int anisotropic;
	anisotropicCombo->Append(_("Off"));
	anisotropicCombo->Append(_T(" 1x"));
	anisotropicCombo->Append(_T(" 2x"));
	anisotropicCombo->Append(_T(" 4x"));
	anisotropicCombo->Append(_T(" 8x"));
	anisotropicCombo->Append(_T("16x"));
	proman->Get()->Read(PRO_CFG_VIDEO_ANISOTROPIC, &anisotropic, 0);
	switch(anisotropic) {
		case 1:
			anisotropic = 1;
			break;
		case 2:
			anisotropic = 2;
			break;
		case 4:
			anisotropic = 3;
			break;
		case 8:
			anisotropic = 4;
			break;
		case 16:
			anisotropic = 5;
			break;
		default:
			anisotropic = 0;
	}
	anisotropicCombo->SetSelection(anisotropic);


	wxStaticText* aaText = new wxStaticText(this, wxID_ANY, _("Anti-Alias:"));
	wxChoice* aaCombo = new wxChoice(this, ID_AA_COMBO);
	int antialias;
	aaCombo->Append(_("Off"));
	aaCombo->Append(_T(" 1x"));
	aaCombo->Append(_T(" 2x"));
	aaCombo->Append(_T(" 4x"));
	aaCombo->Append(_T(" 8x"));
	aaCombo->Append(_T("16x"));
	proman->Get()->Read(PRO_CFG_VIDEO_ANISOTROPIC, &antialias, 0);
	switch(antialias) {
		case 1:
			antialias = 1;
			break;
		case 2:
			antialias = 2;
			break;
		case 4:
			antialias = 3;
			break;
		case 8:
			antialias = 4;
			break;
		case 16:
			antialias = 5;
			break;
		default:
			antialias = 0;
	}
	aaCombo->SetSelection(antialias);

	wxStaticText* gsText = 
		new wxStaticText(this, wxID_ANY, _("General settings (recommend: highest):"));
	wxChoice* gsCombo = new wxChoice(this, ID_GS_COMBO);
	gsCombo->Append(_("1. Lowest"));
	gsCombo->Append(_("2. Low"));
	gsCombo->Append(_("3. High"));
	gsCombo->Append(_("4. Highest"));
	int gamespeed;
	proman->Get()->Read(PRO_CFG_VIDEO_GENERAL_SETTINGS, &gamespeed, 3);
	gsCombo->SetSelection(gamespeed);

	wxCheckBox* largeTextureCheck = 
		new wxCheckBox(this, ID_LARGE_TEXTURE_CHECK, _("Use large textures"));
	bool largeTextures;
	proman->Get()->Read(PRO_CFG_VIDEO_USE_LARGE_TEXTURES, &largeTextures, false);
	largeTextureCheck->SetValue(largeTextures);
	wxCheckBox* fontDistortion = 
		new wxCheckBox(this, ID_FONT_DISTORTION_CHECK, _("Fix font distortion"));
	bool fixFont;
	proman->Get()->Read(PRO_CFG_VIDEO_USE_LARGE_TEXTURES, &fixFont, false);
	fontDistortion->SetValue(fixFont);
	fontDistortion->Disable(); // DirectX only
	
	// Sizer for graphics, resolution, depth, etc
	wxGridSizer* videoSizer1 = new wxFlexGridSizer(4); 
	videoSizer1->Add(graphicsText);
	videoSizer1->Add(graphicsCombo);
	videoSizer1->Add(resolutionText);
	videoSizer1->Add(resolutionCombo);
	videoSizer1->Add(depthText);
	videoSizer1->Add(depthCombo);
	videoSizer1->Add(textureFilterText);
	videoSizer1->Add(textureFilterCombo);
	videoSizer1->Add(anisotropicText);
	videoSizer1->Add(anisotropicCombo);
	videoSizer1->Add(aaText);
	videoSizer1->Add(aaCombo);

	wxBoxSizer* videoSizergs = new wxBoxSizer(wxHORIZONTAL);
	videoSizergs->Add(gsText);
	videoSizergs->Add(gsCombo);

	wxBoxSizer* videoSizer3 = new wxBoxSizer(wxHORIZONTAL);
	videoSizer3->Add(largeTextureCheck);
	videoSizer3->Add(fontDistortion);

	wxStaticBoxSizer* videoSizer = new wxStaticBoxSizer(videoBox, wxVERTICAL);
	videoSizer->SetMinSize(wxSize(300, -1));
	videoSizer->Add(videoSizer1);
	videoSizer->Add(videoSizergs);
	videoSizer->Add(videoSizer3);

	// Speech
	wxStaticBox* speechBox = new wxStaticBox(this, wxID_ANY, _("Speech"));
	wxTextCtrl* speechTestText = new wxTextCtrl(this, ID_SPEECH_TEST_TEXT,
		_("Press play to test this string"),
		wxDefaultPosition, wxDefaultSize,
		wxTE_MULTILINE);
	wxChoice* speechVoiceCombo = new wxChoice(this, ID_SPEECH_VOICE_COMBO);
	wxSlider* speechVoiceVolume = 
		new wxSlider(this, ID_SPEECH_VOICE_VOLUME, 50, 0, 100);
	wxButton* speechPlayButton = 
		new wxButton(this, ID_SPEECH_PLAY_BUTTON, _("Play String"));
	wxStaticText* speechUseInText = 
		new wxStaticText(this, wxID_ANY, _("Use simulated speech:"));
	wxCheckBox* speechInTechroomCheck = 
		new wxCheckBox(this, ID_SPEECH_IN_TECHROOM, _("Techroom"));
	wxCheckBox* speechInBriefingCheck = 
		new wxCheckBox(this, ID_SPEECH_IN_BRIEFING, _("Briefings"));
	wxCheckBox* speechInGameCheck = 
		new wxCheckBox(this, ID_SPEECH_IN_GAME, _("Ingame"));
	wxCheckBox* speechInMultiCheck=
		new wxCheckBox(this, ID_SPEECH_IN_MULTI, _("Multiplayer"));

	wxButton* speechMoreVoicesButton = 
		new wxButton(this, ID_SPEECH_MORE_VOICES_BUTTON, _("Get More Voices"));

	wxBoxSizer* speechLeftSizer = new wxBoxSizer(wxVERTICAL);
	speechLeftSizer->Add(speechTestText, wxSizerFlags().Expand());
	speechLeftSizer->Add(speechVoiceCombo);
	speechLeftSizer->Add(speechVoiceVolume, wxSizerFlags().Expand());
	speechLeftSizer->Add(speechPlayButton, wxSizerFlags().Center());

	wxBoxSizer* speechRightSizer = new wxBoxSizer(wxVERTICAL);
	speechRightSizer->Add(speechUseInText);
	speechRightSizer->Add(speechInTechroomCheck);
	speechRightSizer->Add(speechInBriefingCheck);
	speechRightSizer->Add(speechInGameCheck);
	speechRightSizer->Add(speechInMultiCheck);
	speechRightSizer->Add(speechMoreVoicesButton);

	wxStaticBoxSizer* speechSizer = new wxStaticBoxSizer(speechBox, wxHORIZONTAL);
	speechSizer->Add(speechLeftSizer);
	speechSizer->Add(speechRightSizer);

	if ( SpeechMan::WasBuiltIn() && SpeechMan::Initialize() ) {

		speechVoiceCombo->Append(SpeechMan::EnumVoices());
		int speechVoice;
		int speechSystemVoice = SpeechMan::GetVoice();
		if ( speechSystemVoice < 0 ) {
			wxLogWarning(_T("Had problem retriving the system voice, using voice 0"));
			speechSystemVoice = 0;
		}
		// set the voice to what is in the profile, if not set in profile use
		// system settings
		proman->Get()->Read(PRO_CFG_SPEECH_VOICE, &speechVoice, speechSystemVoice);
		// there should not be more than MAX_INT voices installed on a system so
		// the cast of an unsigned int to a signed int should not result in a 
		// loss of data.
		if ( speechVoice >= static_cast<int>(speechVoiceCombo->GetCount()) ) {
			wxLogWarning(_T("Profile speech voice index out of range,")
				_T(" setting to system default"));
			speechVoice = speechSystemVoice;
		}
		speechVoiceCombo->SetSelection(speechVoice);

		int speechVolume;
		int speechSystemVolume = SpeechMan::GetVolume();
		if (speechSystemVolume < 0) {
			wxLogWarning(_T("Had problem in retriving the system speech volume,")
				_T(" setting to 50"));
			speechSystemVolume = 50;
		}
		proman->Get()->Read(PRO_CFG_SPEECH_VOLUME, &speechVolume, speechSystemVolume);
		if ( speechVolume < 0 || speechVolume > 100 ) {
			wxLogWarning(_T("Speech Volume recorded in profile is out of range,")
				_T(" resetting to 50"));
			speechVolume = 50;
		}
		speechVoiceVolume->SetValue(speechVolume);


		bool speechInTechroom;
		proman->Get()->Read(PRO_CFG_SPEECH_IN_TECHROOM, &speechInTechroom, true);
		speechInTechroomCheck->SetValue(speechInTechroom);

		bool speechInBriefings;
		proman->Get()->Read(PRO_CFG_SPEECH_IN_BRIEFINGS, &speechInBriefings, true);
		speechInBriefingCheck->SetValue(speechInBriefings);

		bool speechInGame;
		proman->Get()->Read(PRO_CFG_SPEECH_IN_GAME, &speechInGame, true);
		speechInGameCheck->SetValue(speechInGame);

		bool speechInMulti;
		proman->Get()->Read(PRO_CFG_SPEECH_IN_MULTI, &speechInMulti, true);
		speechInMultiCheck->SetValue(speechInMulti);
	} else {
		speechBox->Disable();
		speechTestText->Disable();
		speechVoiceCombo->Disable();
		speechVoiceVolume->Disable();
		speechPlayButton->Disable();
		speechUseInText->Disable();
		speechInTechroomCheck->Disable();
		speechInBriefingCheck->Disable();
		speechInGameCheck->Disable();
		speechInMultiCheck->Disable();
		speechMoreVoicesButton->Disable();
	}

	// Network
	wxStaticBox* networkBox = new wxStaticBox(this, wxID_ANY, _("Network"));

	wxChoice* networkType = new wxChoice(this, ID_NETWORK_TYPE);
	networkType->Append(_T("None"));
	networkType->Append(_T("Dialup"));
	networkType->Append(_T("LAN/Direct Connection"));
	wxString type;
	proman->Get()->Read(PRO_CFG_NETWORK_TYPE, &type, _T("None"));
	networkType->SetStringSelection(type);
	wxChoice* networkSpeed = new wxChoice(this, ID_NETWORK_SPEED);
	networkSpeed->Append(_T("None"));
	networkSpeed->Append(_T("Slow"));
	networkSpeed->Append(_T("56K"));
	networkSpeed->Append(_T("ISDN"));
	networkSpeed->Append(_T("Cable"));
	networkSpeed->Append(_T("Fast"));
	wxString speed;
	proman->Get()->Read(PRO_CFG_NETWORK_SPEED, &speed, _T("None"));
	networkSpeed->SetStringSelection(speed);

	wxTextCtrl* networkPort = 
		new wxTextCtrl(this, ID_NETWORK_PORT, wxEmptyString);
	int port;
	proman->Get()->Read(PRO_CFG_NETWORK_PORT, &port, 0);
	networkPort->SetValue(wxString::Format(_T("%d"), port));
	networkPort->SetValidator(wxTextValidator(wxFILTER_NUMERIC));
	wxTextCtrl* networkIP = new wxTextCtrl(this, ID_NETWORK_IP, wxEmptyString);
	wxString ip;
	proman->Get()->Read(PRO_CFG_NETWORK_IP, &ip, _T(""));
	networkIP->SetValue(ip);

	wxGridSizer* networkInsideSizer = new wxFlexGridSizer(4);
	networkInsideSizer->Add(
		new wxStaticText(this, wxID_ANY, _("Connection type:")));
	networkInsideSizer->Add(networkType);
	networkInsideSizer->Add(
		new wxStaticText(this, wxID_ANY, _("Port:")));
	networkInsideSizer->Add(networkPort);
	networkInsideSizer->Add(
		new wxStaticText(this, wxID_ANY, _("Connection speed:")));
	networkInsideSizer->Add(networkSpeed);
	networkInsideSizer->Add(
		new wxStaticText(this, wxID_ANY, _("IP:")));
	networkInsideSizer->Add(networkIP);

	wxStaticBoxSizer* networkSizer = 
		new wxStaticBoxSizer(networkBox, wxVERTICAL);
	networkSizer->Add(networkInsideSizer);

	// Audio
	wxStaticBox* audioBox = new wxStaticBox(this, wxID_ANY, _("Audio"));

	this->soundDeviceText = new wxStaticText(this, wxID_ANY, _("Sound device:"));
	this->soundDeviceCombo = new wxChoice(this, ID_SELECT_SOUND_DEVICE);

	this->openALVersion = new wxStaticText(this, wxID_ANY, wxEmptyString);
	openALVersion->Wrap(153); /* HACKHACK: hard coded width, using number of
							  pixels wide the text is on the prototype.*/
	this->downloadOpenALButton = new wxButton(this, ID_DOWNLOAD_OPENAL, _("Download OpenAL"));
	this->detectOpenALButton = new wxButton(this, ID_DETECT_OPENAL, _("Detect"));

	wxStaticBoxSizer* audioSizer = new wxStaticBoxSizer(audioBox, wxVERTICAL);
	audioSizer->Add(soundDeviceText);
	audioSizer->Add(soundDeviceCombo, wxSizerFlags().Expand());
	audioSizer->Add(openALVersion, wxSizerFlags().Center());
	audioSizer->Add(downloadOpenALButton, wxSizerFlags().Center());
	audioSizer->Add(detectOpenALButton, wxSizerFlags().Center());

	// fill in controls
	this->SetupOpenALSection();

	// Joystick
	wxStaticBox* joystickBox = new wxStaticBox(this, wxID_ANY, _("Joystick"));

	wxStaticText* selectedJoystickText = new wxStaticText(this, wxID_ANY, _("Selected joystick:"));
	this->joystickSelected = new wxChoice(this, ID_JOY_SELECTED);
	this->joystickForceFeedback = new wxCheckBox(this, ID_JOY_FORCE_FEEDBACK, _("Force feedback"));
	this->joystickDirectionalHit = new wxCheckBox(this, ID_JOY_DIRECTIONAL_HIT, _("Directional hit"));
	this->joystickCalibrateButton = new wxButton(this, ID_JOY_CALIBRATE_BUTTON, _("Calibrate"));
	this->joystickDetectButton = new wxButton(this, ID_JOY_DETECT_BUTTON, _("Detect"));

	this->SetupJoystickSection();

	wxBoxSizer* joyButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	joyButtonSizer->Add(joystickCalibrateButton);
	joyButtonSizer->Add(joystickDetectButton);

	wxStaticBoxSizer* joystickSizer = new wxStaticBoxSizer(joystickBox, wxVERTICAL);
	joystickSizer->Add(selectedJoystickText);
	joystickSizer->Add(joystickSelected, wxSizerFlags().Expand());
	joystickSizer->Add(joystickForceFeedback);
	joystickSizer->Add(joystickDirectionalHit);
	joystickSizer->Add(joyButtonSizer);

	// Proxy
	wxStaticBox* proxyBox = new wxStaticBox(this, wxID_ANY, _("Proxy"));

	wxChoicebook* proxyChoice = new ProxyChoice(this, ID_PROXY_TYPE);

	wxStaticBoxSizer* proxySizer = new wxStaticBoxSizer(proxyBox, wxVERTICAL);
	proxySizer->Add(proxyChoice, wxSizerFlags().Expand());

	// Final Layout
	wxBoxSizer* leftColumnSizer = new wxBoxSizer(wxVERTICAL);
	leftColumnSizer->Add(videoSizer, wxSizerFlags().Expand());
	leftColumnSizer->Add(speechSizer, wxSizerFlags().Expand());
	leftColumnSizer->Add(networkSizer, wxSizerFlags().Expand());

	wxBoxSizer* rightColumnSizer = new wxBoxSizer(wxVERTICAL);
	rightColumnSizer->Add(audioSizer, wxSizerFlags().Expand());
	rightColumnSizer->Add(joystickSizer, wxSizerFlags().Expand());
	rightColumnSizer->Add(proxySizer, wxSizerFlags().Expand());

	wxBoxSizer* columnsSizer = new wxBoxSizer(wxHORIZONTAL);
	columnsSizer->Add(leftColumnSizer);
	columnsSizer->Add(rightColumnSizer);

	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	exeSizer->SetMinSize(TAB_AREA_WIDTH, -1);
	sizer->Add(exeSizer);
	sizer->Add(columnsSizer);

	this->SetSizer(sizer);
	this->Layout();

}

BasicSettingsPage::~BasicSettingsPage() {
	TCManager::DeInitialize();
	if ( SpeechMan::IsInitialized() ) {
		SpeechMan::DeInitialize();
	}
	JoyMan::DeInitialize();
	OpenALMan::DeInitialize();
}

/// Event Handling
BEGIN_EVENT_TABLE(BasicSettingsPage, wxPanel)
EVT_BUTTON(ID_EXE_SELECT_ROOT_BUTTON, BasicSettingsPage::OnSelectTC)
EVT_CHOICE(ID_EXE_CHOICE_BOX, BasicSettingsPage::OnSelectExecutable)
EVT_COMMAND( wxID_NONE, EVT_TC_CHANGED, BasicSettingsPage::OnTCChanged)

// Video controls
EVT_CHOICE(ID_GRAPHICS_COMBO, BasicSettingsPage::OnSelectGraphicsAPI)
EVT_CHOICE(ID_RESOLUTION_COMBO, BasicSettingsPage::OnSelectVideoResolution)
EVT_CHOICE(ID_DEPTH_COMBO, BasicSettingsPage::OnSelectVideoDepth)
EVT_CHOICE(ID_TEXTURE_FILTER_COMBO, BasicSettingsPage::OnSelectVideoTextureFilter)
EVT_CHOICE(ID_ANISOTROPIC_COMBO, BasicSettingsPage::OnSelectVideoAnistropic)
EVT_CHOICE(ID_AA_COMBO, BasicSettingsPage::OnSelectVideoAntiAlias)
EVT_CHOICE(ID_GS_COMBO, BasicSettingsPage::OnSelectVideoGeneralSettings)
EVT_CHECKBOX(ID_LARGE_TEXTURE_CHECK, BasicSettingsPage::OnToggleVideoUseLargeTexture)
EVT_CHECKBOX(ID_FONT_DISTORTION_CHECK, BasicSettingsPage::OnToggleVideoFixFontDistortion)

// Speech Controls
EVT_CHOICE(ID_SPEECH_VOICE_COMBO, BasicSettingsPage::OnSelectSpeechVoice)
EVT_SLIDER(ID_SPEECH_VOICE_VOLUME, BasicSettingsPage::OnChangeSpeechVolume)
EVT_BUTTON(ID_SPEECH_PLAY_BUTTON, BasicSettingsPage::OnPlaySpeechText)
EVT_CHECKBOX(ID_SPEECH_IN_TECHROOM, BasicSettingsPage::OnToggleSpeechInTechroom)
EVT_CHECKBOX(ID_SPEECH_IN_BRIEFING, BasicSettingsPage::OnToggleSpeechInBriefing)
EVT_CHECKBOX(ID_SPEECH_IN_GAME, BasicSettingsPage::OnToggleSpeechInGame)
EVT_BUTTON(ID_SPEECH_MORE_VOICES_BUTTON, BasicSettingsPage::OnGetMoreVoices)

// Network
EVT_CHOICE(ID_NETWORK_TYPE, BasicSettingsPage::OnSelectNetworkType)
EVT_CHOICE(ID_NETWORK_SPEED, BasicSettingsPage::OnSelectNetworkSpeed)
EVT_TEXT(ID_NETWORK_PORT, BasicSettingsPage::OnChangePort)
EVT_TEXT(ID_NETWORK_IP, BasicSettingsPage::OnChangeIP)

// OpenAL
EVT_CHOICE(ID_SELECT_SOUND_DEVICE, BasicSettingsPage::OnSelectOpenALDevice)
EVT_BUTTON(ID_DOWNLOAD_OPENAL, BasicSettingsPage::OnDownloadOpenAL)
EVT_BUTTON(ID_DETECT_OPENAL, BasicSettingsPage::OnDetectOpenAL)

// Joystick
EVT_CHOICE(ID_JOY_SELECTED, BasicSettingsPage::OnSelectJoystick)
EVT_CHECKBOX(ID_JOY_FORCE_FEEDBACK, BasicSettingsPage::OnCheckForceFeedback)
EVT_CHECKBOX(ID_JOY_DIRECTIONAL_HIT, BasicSettingsPage::OnCheckDirectionalHit)
EVT_BUTTON(ID_JOY_CALIBRATE_BUTTON, BasicSettingsPage::OnCalibrateJoystick)
EVT_BUTTON(ID_JOY_DETECT_BUTTON, BasicSettingsPage::OnDetectJoystick)

// Profile
EVT_COMMAND(wxID_NONE, EVT_CURRENT_PROFILE_CHANGED, BasicSettingsPage::ProfileChanged)

END_EVENT_TABLE()

void BasicSettingsPage::OnSelectTC(wxCommandEvent &WXUNUSED(event)) {
	wxString directory;
	ProMan* proman = ProMan::GetProfileManager();
	proman->Get()->Read(PRO_CFG_TC_ROOT_FOLDER, &directory, wxEmptyString);
	wxDirDialog filechooser(this, _T("Please choose the base directory of the Total Conversion"),
		directory, wxDD_DEFAULT_STYLE|wxDD_DIR_MUST_EXIST);

	wxString chosenDirectory;
	wxFileName path;
	while (true) {
		if ( wxID_CANCEL == filechooser.ShowModal() ) {
			return;
		}
		chosenDirectory = filechooser.GetPath();
		if ( chosenDirectory == directory ) {
			wxLogInfo(_T("The exe folder selection was not changed."));
			return; // User canceled, bail out.
		}
		path.SetPath(chosenDirectory);
		if ( !path.IsOk() ) {
			wxLogWarning(_T("Directory is not valid"));
			continue;
		} else if ( FSOExecutable::CheckRootFolder(path) ) {
			break;
		} else {
			wxLogWarning(_T("Directory does not have supported executables in it"));
		}
	}
	wxLogDebug(_T("User choose '%s' as the TC directory"), path.GetPath().c_str());
	proman->Get()->Write(PRO_CFG_TC_ROOT_FOLDER, path.GetPath());
	TCManager::GenerateTCChanged();
}

/** Handles TCChanged events from TCManager.

Currently function only changes the executable dropbox control (clearing, and 
filling in the executables that are in the new TC folder) and removes the
currently select executable from the active profile only if the executable
specified in the profile does not exist in the TC.

\note clearing the selected executable disables the play button.
\note Emits a EVT_TC_BINARY_CHANGED in any case.*/
void BasicSettingsPage::OnTCChanged(wxCommandEvent &WXUNUSED(event)) {

	ExeChoice *exeChoice = dynamic_cast<ExeChoice*>(
		wxWindow::FindWindowById(ID_EXE_CHOICE_BOX, this));
	wxCHECK_RET( exeChoice != NULL, 
		_T("Cannot find executable choice control"));

	wxTextCtrl* tcFolder = dynamic_cast<wxTextCtrl*>(
		wxWindow::FindWindowById(ID_EXE_ROOT_FOLDER_BOX, this));
	wxCHECK_RET( tcFolder != NULL, 
		_T("Cannot find Text Control to show folder in."));

	wxString tcPath, binaryName;
	exeChoice->Clear();

	if ( ProMan::GetProfileManager()->Get()
			->Read(PRO_CFG_TC_ROOT_FOLDER, &tcPath) ) {
		tcFolder->SetValue(tcPath);

		this->FillExecutableDropBox(exeChoice, wxFileName(tcPath, wxEmptyString));
		exeChoice->Enable();

		/* check to see if the exe listed in the profile actually does exist in
		the list */
		ProMan::GetProfileManager()->Get()
			->Read(PRO_CFG_TC_CURRENT_BINARY, &binaryName);
		if ( !exeChoice->FindAndSetSelectionWithClientData(binaryName) ) {
			ProMan::GetProfileManager()->Get()
				->DeleteEntry(PRO_CFG_TC_CURRENT_BINARY);
		}
	}
	this->GetSizer()->Layout();
	TCManager::GenerateTCBinaryChanged();
}

/** Puts the pretty description of all of the executables in the TCs folder
into the Executable DropBox.  This function does nothing else to the choice
control, not even clearing the drop box (call the Clear function if you don't
want the old items to stay. */
void BasicSettingsPage::FillExecutableDropBox(wxChoice* exeChoice, wxFileName path) {
	wxArrayString exes = FSOExecutable::GetBinariesFromRootFolder(path);
	wxArrayString::iterator iter = exes.begin();
	while ( iter != exes.end() ) {
		wxFileName path(*iter);
		FSOExecutable ver = FSOExecutable::GetBinaryVersion(path.GetFullName());
		exeChoice->Insert(FSOExecutable::MakeVersionStringFromVersion(ver), 0, new FSOExecutable(ver));
		iter++;
	}
}

void BasicSettingsPage::OnSelectExecutable(wxCommandEvent &WXUNUSED(event)) {
	ExeChoice* choice = dynamic_cast<ExeChoice*>(
		wxWindow::FindWindowById(ID_EXE_CHOICE_BOX, this));
	wxCHECK_RET( choice != NULL, 
		_T("OnSelectExecutable: cannot find choice drop box"));

	FSOExecutable* ver = dynamic_cast<FSOExecutable*>(
		choice->GetClientObject(choice->GetSelection()));
	wxCHECK_RET( ver != NULL,
		_T("OnSelectExecutable: choice does not have FSOVersion data"));
	wxLogDebug(_T("Have selected ver for %s"), ver->GetExecutableName().c_str());

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_TC_CURRENT_BINARY, ver->GetExecutableName());
	TCManager::GenerateTCBinaryChanged();
}

void BasicSettingsPage::OnSelectGraphicsAPI(wxCommandEvent &WXUNUSED(event)) {
	wxChoice* api = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(ID_GRAPHICS_COMBO, this));
	wxCHECK_RET( api != NULL, _T("Cannot find graphics api choice box"));

	wxStaticBox* box = dynamic_cast<wxStaticBox*>(
		wxWindow::FindWindowById(ID_VIDEO_STATIC_BOX, this));
	wxCHECK_RET( box != NULL, _T("Cannot find static box for video settings"));
	
	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_VIDEO_API, api->GetStringSelection());
	box->SetLabel(wxString::Format(_("Video (%s)"), api->GetStringSelection().c_str()));
}

class Resolution: public wxClientData {
public:
	Resolution(int height, int width) {
		this->height = height;
		this->width = width;
	}
	virtual ~Resolution() {}
	int GetHeight() { return this->height; }
	int GetWidth() { return this->width; }
private:
	int height;
	int width;
};

void BasicSettingsPage::FillResolutionDropBox(wxChoice *exeChoice) {
#ifdef WIN32
	DEVMODE deviceMode;
	DWORD modeCounter = 0;
	BOOL result;

	wxLogDebug(_T("Enumerating graphics modes"));

	do {
		memset(&deviceMode, 0, sizeof(DEVMODE));
		deviceMode.dmSize = sizeof(DEVMODE);

		result = EnumDisplaySettings(NULL, modeCounter, &deviceMode);

		if ( result == TRUE ) {
			wxLogDebug(_T(" %dx%d %d bit %d hertz (%d)"),
				deviceMode.dmPelsWidth,
				deviceMode.dmPelsHeight,
				deviceMode.dmBitsPerPel,
				deviceMode.dmDisplayFrequency,
				deviceMode.dmDisplayFlags);
			wxString resolution = wxString::Format(
				CFG_RES_FORMAT_STRING, deviceMode.dmPelsWidth, deviceMode.dmPelsHeight);
			// check to see if the resolution has already been added.
			wxArrayString strings = exeChoice->GetStrings();
			wxArrayString::iterator iter = strings.begin();
			bool exists = false;
			while ( iter != strings.end() ) {
				if ( *iter == resolution ) {
					exists = true;
				}
				iter++;
			}
			if ( !exists ) {
				exeChoice->Insert(
					resolution,
					0,
					new Resolution(deviceMode.dmPelsHeight, deviceMode.dmPelsWidth));
			}
		}
		modeCounter++;
	} while ( result == TRUE );
#elif HAS_SDL == 1
	wxLogDebug(_T("Enumerating graphics modes with SDL"));
	SDL_Rect** modes;
	modes = SDL_ListModes(NULL, SDL_FULLSCREEN|SDL_HWSURFACE);
	
	if ( modes == (SDL_Rect**)NULL ) {
	  wxLogWarning(_T("Unable retreive any video modes"));
	} else if ( modes == (SDL_Rect**)(-1) ) {
	  wxLogWarning(_T("All resolutions are available.  If you get this message please report it to the developers as they do not think this response is actually possible"));
	} else {
	  wxLogDebug(_T("Found the following video modes:"));
	  
	  for(int i = 0; modes[i]; i++) {
	    wxLogDebug(_T(" %d x %d"), modes[i]->w, modes[i]->h);
	    
	    wxString resolution = wxString::Format(CFG_RES_FORMAT_STRING, modes[i]->w, modes[i]->h);
	    
	    /*while ( iter != strings.end() ) {
		    if ( *iter == resolution ) {
			    exists = true;
		    }
		    iter++;
	    }
	    if ( !exists ) {*/
	    {
		    exeChoice->Insert(
			    resolution,
			    0,
			    new Resolution(modes[i]->w, modes[i]->h));
	    }
	  }
	}
#else
#error "BasicSettingsPage::FillResolutionDropBox not implemented because not on windows and SDL is not implemented"
#endif
}

void BasicSettingsPage::OnSelectVideoResolution(wxCommandEvent &WXUNUSED(event)) {
	wxChoice* choice = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(ID_RESOLUTION_COMBO, this));
	wxCHECK_RET( choice != NULL, _T("Unable to find resolution combo"));

	Resolution* res = dynamic_cast<Resolution*>(
		choice->GetClientObject(choice->GetSelection()));
	wxCHECK_RET( res != NULL, _T("Choice does not have Resolution objects"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_VIDEO_RESOLUTION_WIDTH, res->GetWidth());
	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_VIDEO_RESOLUTION_HEIGHT, res->GetHeight());
}

void BasicSettingsPage::OnSelectVideoDepth(wxCommandEvent &WXUNUSED(event)) {
	wxChoice* depth = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(ID_RESOLUTION_COMBO, this));
	wxCHECK_RET( depth != NULL, _T("Unable to find depth choice box"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_VIDEO_BIT_DEPTH,
		(depth->GetSelection() == 0) ? 16 : 32);
}

void BasicSettingsPage::OnSelectVideoTextureFilter(wxCommandEvent &WXUNUSED(event)) {
	wxChoice* tex = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(ID_TEXTURE_FILTER_COMBO, this));
	wxCHECK_RET( tex != NULL, _T("Unable to find texture filter choice"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_VIDEO_TEXTURE_FILTER,
		(tex->GetSelection() == 0) ? _T("Bilinear") : _T("Trilinear"));
}

void BasicSettingsPage::OnSelectVideoAnistropic(wxCommandEvent &WXUNUSED(event)) {
	wxChoice* as = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(ID_ANISOTROPIC_COMBO, this));
	wxCHECK_RET( as != NULL, _T("Unable to find anisotropic choice"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_VIDEO_ANISOTROPIC,
		(as->GetSelection() == 0) ? 0 : 1 << as->GetSelection());
}

void BasicSettingsPage::OnSelectVideoAntiAlias(wxCommandEvent &WXUNUSED(event)) {
	wxChoice* aa = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(ID_AA_COMBO, this));
	wxCHECK_RET( aa != NULL, _T("Unable to find anti-alias choice"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_VIDEO_ANTI_ALIAS, (aa->GetSelection() == 0) ? 0 : 1 << aa->GetSelection());

}

void BasicSettingsPage::OnSelectVideoGeneralSettings(wxCommandEvent &WXUNUSED(event)) {
	wxChoice* gs = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(ID_GS_COMBO, this));
	wxCHECK_RET( gs != NULL, _T("Unable to find general settings choice"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_VIDEO_GENERAL_SETTINGS, gs->GetSelection());
}

void BasicSettingsPage::OnToggleVideoUseLargeTexture(wxCommandEvent &WXUNUSED(event)) {
	wxCheckBox* large = dynamic_cast<wxCheckBox*>(
		wxWindow::FindWindowById(ID_LARGE_TEXTURE_CHECK));
	wxCHECK_RET( large != NULL, _T("Unable to find large texture checkbox"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_VIDEO_USE_LARGE_TEXTURES,  large->IsChecked());
}

void BasicSettingsPage::OnToggleVideoFixFontDistortion(wxCommandEvent &WXUNUSED(event)) {
	wxCheckBox* font = dynamic_cast<wxCheckBox*>(
		wxWindow::FindWindowById(ID_FONT_DISTORTION_CHECK));
	wxCHECK_RET( font != NULL, _T("Unable to find font distortion checkbox"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_VIDEO_FIX_FONT_DISTORTION, font->IsChecked());
}

void BasicSettingsPage::OnSelectSpeechVoice(wxCommandEvent &WXUNUSED(event)) {
	wxCHECK_RET( SpeechMan::WasBuiltIn(), _T("Speech was not complied in."));
	wxChoice* voice = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(ID_SPEECH_VOICE_COMBO, this));
	wxCHECK_RET( voice != NULL, _T("Unable to find voice choice box"));

	int v = voice->GetSelection();
	
	SpeechMan::SetVoice(v);

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_SPEECH_VOICE, v);
}

void BasicSettingsPage::OnChangeSpeechVolume(wxCommandEvent &WXUNUSED(event)) {
	wxCHECK_RET( SpeechMan::WasBuiltIn(), _T("Speech was not complied in."));
	wxSlider* volume = dynamic_cast<wxSlider*>(
		wxWindow::FindWindowById(ID_SPEECH_VOICE_VOLUME, this));
	wxCHECK_RET( volume != NULL, _T("Unable to find speech volume slider"));

	int v = volume->GetValue();

	SpeechMan::SetVolume(v);

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_SPEECH_VOLUME, v);
}

void BasicSettingsPage::OnPlaySpeechText(wxCommandEvent &WXUNUSED(event)) {
	wxCHECK_RET( SpeechMan::WasBuiltIn(), _T("Speech was not complied in."));
	wxTextCtrl *text = dynamic_cast<wxTextCtrl*>(
		wxWindow::FindWindowById(ID_SPEECH_TEST_TEXT, this));
	wxCHECK_RET( text != NULL, _T("Unable to find text control to get play text"));

	wxString str = text->GetValue();

	SpeechMan::Speak(str);
}

void BasicSettingsPage::OnToggleSpeechInTechroom(wxCommandEvent &event) {
	wxCHECK_RET( SpeechMan::WasBuiltIn(), _T("Speech was not complied in."));
	bool checked = event.IsChecked();

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_SPEECH_IN_TECHROOM, checked);
}

void BasicSettingsPage::OnToggleSpeechInBriefing(wxCommandEvent &event) {
	wxCHECK_RET( SpeechMan::WasBuiltIn(), _T("Speech was not complied in."));
	bool checked = event.IsChecked();

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_SPEECH_IN_BRIEFINGS, checked);
}

void BasicSettingsPage::OnToggleSpeechInGame(wxCommandEvent &event) {
	wxCHECK_RET( SpeechMan::WasBuiltIn(), _T("Speech was not complied in."));
	bool checked = event.IsChecked();

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_SPEECH_IN_GAME, checked);
}

void BasicSettingsPage::OnGetMoreVoices(wxCommandEvent &WXUNUSED(event)) {
	HelpManager::OpenHelpById(ID_SPEECH_MORE_VOICES_BUTTON);
}

void BasicSettingsPage::OnChangeIP(wxCommandEvent &event) {
	wxTextCtrl* ip = dynamic_cast<wxTextCtrl*>(
		wxWindow::FindWindowById(event.GetId(), this));
	wxCHECK_RET(ip != NULL, _T("Unable to find IP Text Control"));

	wxString string(ip->GetValue());

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_NETWORK_IP, string);
}

void BasicSettingsPage::OnChangePort(wxCommandEvent &event) {
	wxTextCtrl* port = dynamic_cast<wxTextCtrl*>(
		wxWindow::FindWindowById(event.GetId(), this));
	wxCHECK_RET(port != NULL, _T("Unable to find Port Text Control"));

	long portNumber;
	if ( port->GetValue().ToLong(&portNumber) ) {
		if ( portNumber < 0 ) {
			wxLogInfo(_T("Port number must be greater than or equal to 0"));
		} else if ( portNumber > 65535 ) {
			wxLogInfo(_T("Port number must be less than 65536"));
		} else {
			int portNumber1 = static_cast<int>(portNumber);
			ProMan::GetProfileManager()->Get()
				->Write(PRO_CFG_NETWORK_PORT, portNumber1);
		}
	} else {
		wxLogWarning(_T("Port number is not a number"));
	}
}

void BasicSettingsPage::OnSelectNetworkSpeed(wxCommandEvent &event) {
	wxChoice* networkSpeed = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(event.GetId(), this));
	wxCHECK_RET(networkSpeed != NULL, _T("Unable to find Network speed choice"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_NETWORK_SPEED, networkSpeed->GetStringSelection());
}

void BasicSettingsPage::OnSelectNetworkType(wxCommandEvent &event) {
	wxChoice* networkType = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(event.GetId(), this));
	wxCHECK_RET(networkType != NULL, _T("Unable to find Network type choice"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_NETWORK_SPEED, networkType->GetStringSelection());
}

void BasicSettingsPage::OnSelectOpenALDevice(wxCommandEvent &event) {
	wxChoice* openaldevice = dynamic_cast<wxChoice*>(
		wxWindow::FindWindowById(event.GetId(), this));
	wxCHECK_RET(openaldevice != NULL, _T("Unable to find OpenAL Device choice"));

	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_OPENAL_DEVICE, openaldevice->GetStringSelection());
}

void BasicSettingsPage::OnDownloadOpenAL(wxCommandEvent &WXUNUSED(event)) {
	this->OpenNonSCPWebSite(_T("http://connect.creativelabs.com/openal/Downloads/Forms/AllItems.aspx"));
}

void BasicSettingsPage::OnDetectOpenAL(wxCommandEvent& WXUNUSED(event)) {
	if ( !OpenALMan::IsInitialized() ) {
		this->SetupOpenALSection();
	}
}

void BasicSettingsPage::SetupOpenALSection() {
	if ( !OpenALMan::WasCompliedIn() ) {
		this->openALVersion->SetLabel(_("Launcher was not compiled to support OpenAL"));
		this->soundDeviceText->Disable();
		this->soundDeviceCombo->Disable();
		this->downloadOpenALButton->Disable();
		this->detectOpenALButton->Disable();
	} else if ( !OpenALMan::Initialize() ) {
		this->openALVersion->SetLabel(_("Unable to initialize OpenAL"));
		this->soundDeviceText->Disable();
		this->soundDeviceCombo->Disable();
		this->detectOpenALButton->SetLabel(_("Redetect OpenAL"));
		this->downloadOpenALButton->Enable();
	} else {
		// have working openal
		this->soundDeviceCombo->Append(OpenALMan::GetAvailiableDevices());
		wxString openaldevice;
		if ( ProMan::GetProfileManager()->Get()
			->Read(PRO_CFG_OPENAL_DEVICE, &openaldevice) ) {
				soundDeviceCombo->SetStringSelection(
					openaldevice);
		} else {
			soundDeviceCombo->SetStringSelection(
				OpenALMan::SystemDefaultDevice());
		}
		this->soundDeviceText->Enable();
		this->soundDeviceCombo->Enable();
		this->openALVersion->SetLabel(OpenALMan::GetCurrentVersion());
		this->downloadOpenALButton->Disable();
		this->detectOpenALButton->Disable();
	}
}

void BasicSettingsPage::OpenNonSCPWebSite(wxString url) {
	::wxLaunchDefaultBrowser(url);
}

/** Client data for the Joystick Choice box. Stores the joysticks
Windows ID so that we can pass it back correctly to the engine. */
class JoyNumber: public wxClientData {
public:
	JoyNumber(unsigned int i) {
		this->number = i;
	}
	int GetNumber() {
		return static_cast<int>(this->number);
	}
private:
	unsigned int number;
};

void BasicSettingsPage::SetupJoystickSection() {
	this->joystickSelected->Clear();
	if ( !JoyMan::WasCompiledIn() ) {
		this->joystickSelected->Disable();
		this->joystickSelected->Append(_("No Launcher Support"));
		this->joystickForceFeedback->Disable();
		this->joystickDirectionalHit->Disable();
		this->joystickCalibrateButton->Disable();
		this->joystickDetectButton->Disable();
	} else if ( !JoyMan::Initialize() ) {
		this->joystickSelected->Disable();
		this->joystickSelected->Append(_("Initialize Failed"));
		this->joystickForceFeedback->Disable();
		this->joystickDirectionalHit->Disable();
		this->joystickCalibrateButton->Disable();
		this->joystickDetectButton->Enable();
	} else {
		this->joystickSelected
			->Append(_("No Joystick"), new JoyNumber(JOYMAN_INVAILD_JOYSTICK));
		for ( unsigned int i = 0; i < JoyMan::NumberOfJoysticks(); i++ ) {
			if ( JoyMan::IsJoystickPluggedIn(i) ) {
				this->joystickSelected
					->Append(JoyMan::JoystickName(i), new JoyNumber(i));
			}
		}

		if ( JoyMan::NumberOfPluggedInJoysticks() == 0 ) {
			this->joystickSelected->SetSelection(0);
			this->joystickSelected->Disable();
			this->joystickForceFeedback->Disable();
			this->joystickDirectionalHit->Disable();
			this->joystickCalibrateButton->Disable();
			this->joystickDetectButton->Enable();
		} else {
			int profileJoystick;
			unsigned int i;
			this->joystickSelected->Enable();
			this->joystickDetectButton->Enable();
			ProMan::GetProfileManager()->Get()
				->Read(PRO_CFG_JOYSTICK_ID,
					&profileJoystick,
					JOYMAN_INVAILD_JOYSTICK);
			// set current joystick
			for ( i = 0; i < this->joystickSelected->GetCount(); i++ ) {
				JoyNumber* data = dynamic_cast<JoyNumber*>(
					this->joystickSelected->GetClientObject(i));
				wxCHECK2_MSG( data != NULL, continue,
					_T("JoyNumber is not the clientObject in joystickSelected"));

				if ( profileJoystick == data->GetNumber() ) {
					this->joystickSelected->SetSelection(i);
					this->SetupControlsForJoystick(i);
					return; // All joystick controls are now setup
				}
			}
			// Getting here means that the joystick is no longer installed
			// or is not plugged in
			if ( JoyMan::IsJoystickPluggedIn(profileJoystick) ) {
				wxLogWarning(_T("Last selected joystick is not plugged in"));
			} else {
				wxLogWarning(_T("Last selected joystick does not seem to be installed"));
			}
			// set to no joystick (the first selection)
			this->joystickSelected->SetSelection(0);
			this->SetupControlsForJoystick(0);
		}
	}
}

void BasicSettingsPage::SetupControlsForJoystick(unsigned int i) {
	JoyNumber* joynumber = dynamic_cast<JoyNumber*>(
		this->joystickSelected->GetClientObject(i));
	wxCHECK_RET( joynumber != NULL,
		_T("JoyNumber is not joystickSelected's client data"));
	if ( JoyMan::HasCalibrateTool(joynumber->GetNumber()) ) {
		this->joystickCalibrateButton->Enable();
	} else {
		this->joystickCalibrateButton->Disable();
	}

	if ( JoyMan::SupportsForceFeedback(joynumber->GetNumber()) ) {
		bool ff, direct;
		ProMan::GetProfileManager()->Get()
			->Read(PRO_CFG_JOYSTICK_DIRECTIONAL, &direct, false);
		ProMan::GetProfileManager()->Get()
			->Read(PRO_CFG_JOYSTICK_FORCE_FEEDBACK, &ff, false);
		this->joystickDirectionalHit->SetValue(direct);
		this->joystickForceFeedback->SetValue(ff);

		this->joystickDirectionalHit->Enable();
		this->joystickForceFeedback->Enable();
	} else {
		this->joystickDirectionalHit->Disable();
		this->joystickForceFeedback->Disable();
	}
	ProMan::GetProfileManager()->Get()
		->Write(PRO_CFG_JOYSTICK_ID, joynumber->GetNumber());
}

void BasicSettingsPage::OnSelectJoystick(
	wxCommandEvent &WXUNUSED(event)) {
	this->SetupControlsForJoystick(
		this->joystickSelected->GetSelection());
}

void BasicSettingsPage::OnCheckForceFeedback(
	wxCommandEvent &event) {
		ProMan::GetProfileManager()->Get()
			->Write(PRO_CFG_JOYSTICK_FORCE_FEEDBACK, event.IsChecked());
}

void BasicSettingsPage::OnCheckDirectionalHit(
	wxCommandEvent &event) {
		ProMan::GetProfileManager()->Get()
			->Write(PRO_CFG_JOYSTICK_DIRECTIONAL, event.IsChecked());
}

void BasicSettingsPage::OnCalibrateJoystick(
	wxCommandEvent &WXUNUSED(event)) {
		JoyNumber *data = dynamic_cast<JoyNumber*>(
			this->joystickSelected->GetClientObject(
			this->joystickSelected->GetSelection()));
		wxCHECK_RET( data != NULL,
			_T("joystickSelected does not have JoyNumber as clientdata"));

		JoyMan::LaunchCalibrateTool(data->GetNumber());
}

void BasicSettingsPage::OnDetectJoystick(wxCommandEvent &WXUNUSED(event)) {
		if ( JoyMan::DeInitialize() ) {
			this->SetupJoystickSection();
		}
}

//////////// ProxyChoice
ProxyChoice::ProxyChoice(wxWindow *parent, wxWindowID id)
:wxChoicebook(parent, id) {
	wxString type;
	ProMan::GetProfileManager()->Global()
		->Read(GBL_CFG_PROXY_TYPE, &type, _T("none"));

	wxPanel* noneProxyPanel = new wxPanel(this);
	this->AddPage(noneProxyPanel, _("None"));

	/// Manual Proxy
	wxPanel* manualProxyPanel = new wxPanel(this);
	wxStaticText* proxyHttpText = new wxStaticText(manualProxyPanel, wxID_ANY, _("HTTP Proxy:"));
	wxString server;
	ProMan::GetProfileManager()->Global()
		->Read(GBL_CFG_PROXY_SERVER, &server, _T(""));
	wxTextCtrl* proxyHttpServer = new wxTextCtrl(manualProxyPanel, ID_PROXY_HTTP_SERVER, server);
	wxStaticText* proxyHttpPortText = new wxStaticText(manualProxyPanel, wxID_ANY, _("Port:"));

	int port;
	ProMan::GetProfileManager()->Global()
		->Read(GBL_CFG_PROXY_PORT, &port, 0);
	wxTextCtrl* proxyHttpPort = new wxTextCtrl(manualProxyPanel, ID_PROXY_HTTP_PORT, wxString::Format(_T("%d"), port));

	wxBoxSizer* manualProxyPortSizer = new wxBoxSizer(wxHORIZONTAL);
	manualProxyPortSizer->Add(proxyHttpPortText);
	manualProxyPortSizer->Add(proxyHttpPort, wxSizerFlags().Proportion(1));

	wxBoxSizer* manualProxySizer = new wxBoxSizer(wxVERTICAL);
	manualProxySizer->Add(proxyHttpText);
	manualProxySizer->Add(proxyHttpServer, wxSizerFlags().Expand());
	manualProxySizer->Add(manualProxyPortSizer, wxSizerFlags().Expand());
	manualProxySizer->SetContainingWindow(this);
	manualProxyPanel->SetSizer(manualProxySizer);

	this->AddPage(manualProxyPanel, _T("Manual"));

	if ( type == _T("manual") ) {
		this->SetSelection(1);
	} else {
		this->SetSelection(0);
	}
}

ProxyChoice::~ProxyChoice() {
}

BEGIN_EVENT_TABLE(ProxyChoice, wxChoicebook)
EVT_TEXT(ID_PROXY_HTTP_SERVER, ProxyChoice::OnChangeServer)
EVT_TEXT(ID_PROXY_HTTP_PORT, ProxyChoice::OnChangePort)
EVT_CHOICEBOOK_PAGE_CHANGED(ID_PROXY_TYPE, ProxyChoice::OnProxyTypeChange)
END_EVENT_TABLE()

void ProxyChoice::OnChangeServer(wxCommandEvent &event) {
	wxString str = event.GetString();

	if ( str == wxEmptyString ) {
		// do nothing
	} else {
		ProMan::GetProfileManager()->Global()->Write(GBL_CFG_PROXY_SERVER, str);
	}
}

void ProxyChoice::OnChangePort(wxCommandEvent &event) {
	int port = event.GetInt();

	ProMan::GetProfileManager()->Global()->Write(GBL_CFG_PROXY_PORT, port);
}

void ProxyChoice::OnProxyTypeChange(wxChoicebookEvent &event) {
	int page = event.GetSelection();
	wxString str;

	switch (page) {
		case 0:
			str = _T("none");
			break;
		case 1:
			str = _T("manual");
			break;
		default:
			wxFAIL_MSG(wxString::Format(_T("Proxy type changed to invalid id %d"), page));
			return;
	}

	ProMan::GetProfileManager()->Global()->Write(GBL_CFG_PROXY_TYPE, str);
}