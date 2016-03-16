#include "stdafx.h"
#include "SpotispyDlg.h"
#include "Spotispy.h"
#include "afxdialogex.h"

#include "UpdateNotifier.h"
#include "UpdateDlg.h"
#include "CommonCOM.h"
#include "Helper.h"

#include "Poco/UnicodeConverter.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CSpotispyDlg::CSpotispyDlg(CWnd* pParent)
	: CDialogEx(IDD_SPOTISPY_DIALOG, pParent) {
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

CSpotispyDlg::~CSpotispyDlg() = default;

void CSpotispyDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CSpotispyDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CSpotispyDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO_MUTEADS, &CSpotispyDlg::OnBnClickedRadioMuteAds)
	ON_BN_CLICKED(IDC_RADIO_LOWVOLADS, &CSpotispyDlg::OnBnClickedRadioLowVolAds)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_SLIDER1, &CSpotispyDlg::OnNMCustomdrawSlider1)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CHECK_SAVE_TWITCHINFO, &CSpotispyDlg::OnBnClickedCheckSaveTwitchInfo)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_TWITCH_FORMAT, &CSpotispyDlg::OnBnClickedButtonSaveTwitchFormat)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_AD_TEXT, &CSpotispyDlg::OnBnClickedButtonSaveAdText)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_STREAM_INFO, &CSpotispyDlg::OnBnClickedButtonSaveStreamInfo)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_CHECK_UPDATE, &CSpotispyDlg::OnNMClickSyslinkCheckUpdate)
END_MESSAGE_MAP()

BOOL CSpotispyDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// TODO: Additional initialization
	m_dlgSave.LoadFromFile();

	SetWindowText(std::wstring{L"Spotispy - " + SPOTISPY_VERSION}.c_str());

	bool muteAdsDefault = true;
	auto muteAds = m_dlgSave.GetOrSaveDefault("muteAds", muteAdsDefault);

	if (muteAds) {
		CheckRadioButton(IDC_RADIO_MUTEADS, IDC_RADIO_LOWVOLADS, IDC_RADIO_MUTEADS);
	}
	else {
		CheckRadioButton(IDC_RADIO_MUTEADS, IDC_RADIO_LOWVOLADS, IDC_RADIO_LOWVOLADS);
	}
	
	auto* volumeSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER1));
	volumeSlider->SetRange(0, 100, true);

	int adVolumeDefault = 10;
	auto adVolume = m_dlgSave.GetOrSaveDefault("adVolume", adVolumeDefault);

	volumeSlider->SetPos(adVolume);

	CString volPercentageText;
	volPercentageText.Format(L"%d", volumeSlider->GetPos());

	auto* volPercentageEdit = static_cast<CEdit*>(GetDlgItem(IDC_VOLPERCENTAGE));
	volPercentageEdit->SetWindowText(volPercentageText);


	auto* muteAdsRadio = static_cast<CButton*>(GetDlgItem(IDC_RADIO_MUTEADS));
	if (muteAdsRadio->GetCheck() == 1) {
		volumeSlider->EnableWindow(false);
	}

	bool saveStreamInfoDefault = false;
	auto saveStreamInfo = m_dlgSave.GetOrSaveDefault("saveStreamInfo", saveStreamInfoDefault);

	auto* saveTwitchInfoCheckBtn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_SAVE_TWITCHINFO));

	if (saveStreamInfo) {
		saveTwitchInfoCheckBtn->SetCheck(1);
	}
	else {
		saveTwitchInfoCheckBtn->SetCheck(0);
	}

	m_saveTwitchInfo = saveTwitchInfoCheckBtn->GetCheck() == 1;

	std::string streamFilePathDefault = "currentlyPlaying.txt";
	auto streamFilePath = m_dlgSave.GetOrSaveDefault("streamFilePath", streamFilePathDefault);

	Poco::UnicodeConverter::toUTF16(streamFilePath, m_twitchFilePath);

	std::string formatStreamTextDefault = "Currently playing: %a - %t [%c / %f]";
	auto formatStreamText = m_dlgSave.GetOrSaveDefault("formatStreamText", formatStreamTextDefault);

	Poco::UnicodeConverter::toUTF16(formatStreamText, m_twitchFormat);

	auto* twitchSavePatternEdit = static_cast<CEdit*>(GetDlgItem(IDC_EDIT_TWITCH_FORMAT));
	twitchSavePatternEdit->SetWindowText(CString{m_twitchFormat.c_str()});

	std::string adStreamTextDefault = "..waiting..";
	auto adStreamText = m_dlgSave.GetOrSaveDefault("adStreamText", adStreamTextDefault);

	Poco::UnicodeConverter::toUTF16(adStreamText, m_adText);

	auto* adTextEdit = static_cast<CEdit*>(GetDlgItem(IDC_EDIT_AD_TEXT));
	adTextEdit->SetWindowText(CString{m_adText.c_str()});

	auto sharedCom = std::make_shared<CommonCOM>();
	if (!sharedCom->IsInitialized()) {
		MessageTerminate(L"Couldn't initialize COM-Library!");
	}

	// End Dialog inits
	m_spotifyAnalyzer = std::make_unique<SpotifyAnalyzer>(AfxGetApp()->m_pMainWnd, sharedCom);
	m_spotifyAnalyzer->SetFocus(true);
	m_spotifyAnalyzer->Analyze();

	m_dlgSave.SaveToFile();

	m_updateNotifier = std::make_unique<UpdateNotifier>(SPOTISPY_VERSION, SPOTISPY_BUILD_DATE, this);

	SetTimer(1, 50, nullptr);
	SetTimer(2, 100, nullptr);	// Play Info
	SetTimer(3, 2000, nullptr);	// UpdateNotifier
	
	return TRUE;
}

// Wenn Sie dem Dialogfeld eine Schaltfläche "Minimieren" hinzufügen, benötigen Sie
//  den nachstehenden Code, um das Symbol zu zeichnen.  Für MFC-Anwendungen, die das 
//  Dokument/Ansicht-Modell verwenden, wird dies automatisch ausgeführt.

void CSpotispyDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CSpotispyDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CSpotispyDlg::OnTimer(UINT nIDEvent) {
	if (nIDEvent == 1) {
		m_spotifyAnalyzer->Analyze();
	}
	else if (nIDEvent == 2) {
		if (m_saveTwitchInfo) {
			auto metaData = m_spotifyAnalyzer->GetMetaData();
			if (metaData != nullptr && m_spotifyAnalyzer->HasFocus() && m_spotifyAnalyzer->IsSpotifyRunning() && !m_spotifyAnalyzer->IsAdPlaying()) {
				SongInfo twitchInfo;
				twitchInfo.m_artist = metaData->m_artistName;
				twitchInfo.m_track = metaData->m_trackName;
				twitchInfo.m_album = metaData->m_albumName;
				twitchInfo.m_length = metaData->m_trackLength;
				twitchInfo.m_playPos = metaData->m_trackPos;

				std::wstring playString = FormatPlayString(m_twitchFormat, twitchInfo);

				if (playString != m_bufferedPlayString) {
					WritePlayString(playString, m_twitchFilePath);
					m_bufferedPlayString = playString;
				}
			}
			else if (m_spotifyAnalyzer->HasFocus() && m_spotifyAnalyzer->IsSpotifyRunning() && m_spotifyAnalyzer->IsAdPlaying()) {
				WritePlayString(m_adText, m_twitchFilePath);
			}
			else if (m_spotifyAnalyzer->HasFocus() && !m_spotifyAnalyzer->IsSpotifyRunning()) {
				WritePlayString(L"", m_twitchFilePath);
			}
		}
	}
	else if (nIDEvent == 3) {
		if (m_updateNotifier->HasData()) {
			UpdateInformation currentVersion;
			currentVersion.m_buildDate = SPOTISPY_BUILD_DATE;
			currentVersion.m_version = SPOTISPY_VERSION;

			// Internally sets HasData to false on fetch
			auto availableVersion = m_updateNotifier->FetchData();

			auto showUpdateInformationDefault = true;
			auto showUpdateInformation = m_dlgSave.GetOrSaveDefault("showUpdateInformation", showUpdateInformationDefault);

			if (currentVersion.m_buildDate != availableVersion.m_buildDate && showUpdateInformation == true) {
				UpdateDlg dialog{currentVersion, availableVersion, L"https://github.com/hatur/Spotispy/releases"};
				auto response = dialog.DoModal();

				if (response == IDOK) {
					
				}
				else if (response == IDCANCEL) {
					m_dlgSave.Insert("showUpdateInformation", false);
				}
			}

			m_dlgSave.SaveToFile();
		}
	}
}

void CSpotispyDlg::OnClose() {
	m_spotifyAnalyzer->RestoreDefaults();

	CDialogEx::OnClose();
}


void CSpotispyDlg::OnBnClickedOk() {
	m_spotifyAnalyzer->RestoreDefaults();

	CDialogEx::OnOK();
}

void CSpotispyDlg::OnBnClickedRadioMuteAds() {
	auto* volumeSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER1));
	volumeSlider->EnableWindow(false);

	m_spotifyAnalyzer->SetAdsBehavior(EAdsBehavior::Mute);

	m_dlgSave.Insert("muteAds", true);
	m_dlgSave.SaveToFile();
}


void CSpotispyDlg::OnBnClickedRadioLowVolAds() {
	auto* volumeSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER1));
	volumeSlider->EnableWindow(true);

	m_spotifyAnalyzer->SetAdsBehavior(EAdsBehavior::LowerVolume);

	m_dlgSave.Insert("muteAds", false);
	m_dlgSave.SaveToFile();
}

void CSpotispyDlg::OnNMCustomdrawSlider1(NMHDR *pNMHDR, LRESULT *pResult) {
	auto* volumeSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER1));

	CString volPercentageText;
	volPercentageText.Format(L"%d", volumeSlider->GetPos());

	auto adVol = volumeSlider->GetPos();

	m_dlgSave.Insert("adVolume", adVol);
	// We don't save it here cause it would be called 100 times, so on crash it might be gone .. TODO?

	auto* volPercentageEdit = static_cast<CEdit*>(GetDlgItem(IDC_VOLPERCENTAGE));
	volPercentageEdit->SetWindowText(volPercentageText);
}

void CSpotispyDlg::OnBnClickedCheckSaveTwitchInfo() {
	auto* saveTwitchInfoCheckBtn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_SAVE_TWITCHINFO));

	m_saveTwitchInfo = saveTwitchInfoCheckBtn->GetCheck() == 1;

	m_dlgSave.Insert("saveStreamInfo", m_saveTwitchInfo);
	m_dlgSave.SaveToFile();
}

bool CSpotispyDlg::GetSaveTwitchInfo() const noexcept {
	return m_saveTwitchInfo;
}

void CSpotispyDlg::MessageTerminate(const CString& quitMessage) const noexcept {
	MessageBoxEx(0, quitMessage, L"Critical error", MB_ICONERROR, 0);
	std::terminate();
}

void CSpotispyDlg::OnBnClickedButtonSaveTwitchFormat() {
	CString twitchFormat;

	GetDlgItemText(IDC_EDIT_TWITCH_FORMAT, twitchFormat);

	m_twitchFormat = std::wstring{twitchFormat};

	std::string formatStr;
	Poco::UnicodeConverter::toUTF8(m_twitchFormat, formatStr);
	m_dlgSave.Insert("formatStreamText", formatStr);
	m_dlgSave.SaveToFile();
}


void CSpotispyDlg::OnBnClickedButtonSaveAdText() {
	CString adText;

	GetDlgItemText(IDC_EDIT_AD_TEXT, adText);

	m_adText = std::wstring{adText};

	std::string adStr;
	Poco::UnicodeConverter::toUTF8(m_adText, adStr);
	m_dlgSave.Insert("adStreamText", adStr);
	m_dlgSave.SaveToFile();
}


void CSpotispyDlg::OnBnClickedButtonSaveStreamInfo() {
	// TODO: Fügen Sie hier Ihren Kontrollbehandlungscode für die Benachrichtigung ein.

	const int bufSize = 512;
	wchar_t currentDirectory[bufSize];

	GetCurrentDirectoryW(bufSize, currentDirectory);

	CFileDialog fileDialog{
		FALSE,
		L"txt",
		L"currentlyPlaying",
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		L"Text File (*.txt)|*.txt||",
		this,
	};

	fileDialog.m_ofn.lpstrInitialDir = currentDirectory;

	if (fileDialog.DoModal() == IDOK) {
		CString fileName = fileDialog.GetFileTitle();

		if (!fileName.IsEmpty()) {
			CString pathName = fileDialog.GetPathName();

			std::wofstream testFile{pathName};
			if (testFile.good()) {
				std::string pathString;

				Poco::UnicodeConverter::toUTF8(pathName, pathString);
				m_dlgSave.Insert("streamFilePath", pathString);
				m_dlgSave.SaveToFile();

				m_twitchFilePath = std::move(pathName);
			}
		}
	}
}


void CSpotispyDlg::OnNMClickSyslinkCheckUpdate(NMHDR *pNMHDR, LRESULT *pResult) {
	ShellExecute(0, L"open", L"https://github.com/hatur/Spotispy", 0, 0, SW_SHOWNORMAL);
}
