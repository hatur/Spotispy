#include "stdafx.h"
#include "Spotispy.h"
#include "SpotispyDlg.h"
#include "afxdialogex.h"
#include "CommonCOM.h"
#include "Helper.h"
#include "TwitchIO.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CSpotispyDlg::CSpotispyDlg(CWnd* pParent)
	: CDialogEx(IDD_SPOTISPY_DIALOG, pParent)
	, m_spotifyAnalyzer{nullptr} {
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

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
END_MESSAGE_MAP()

BOOL CSpotispyDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	// TODO: Additional initialization
	CString windowTitles;

	CheckRadioButton(IDC_RADIO_MUTEADS, IDC_RADIO_LOWVOLADS, IDC_RADIO_MUTEADS);

	auto* volumeSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER1));
	volumeSlider->SetRange(0, 100, true);
	volumeSlider->SetPos(10);

	CString volPercentageText;
	volPercentageText.Format(L"%d", volumeSlider->GetPos());

	auto* volPercentageEdit = static_cast<CEdit*>(GetDlgItem(IDC_VOLPERCENTAGE));
	volPercentageEdit->SetWindowText(volPercentageText);

	auto* muteAdsRadio = static_cast<CButton*>(GetDlgItem(IDC_RADIO_MUTEADS));
	if (muteAdsRadio->GetCheck() == 1) {
		volumeSlider->EnableWindow(false);
	}

	auto* saveTwitchInfoCheckBtn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_SAVE_TWITCHINFO));
	saveTwitchInfoCheckBtn->SetCheck(1);

	auto* twitchSavePatternEdit = static_cast<CEdit*>(GetDlgItem(IDC_EDIT_TWITCH_FORMAT));
	twitchSavePatternEdit->SetWindowText(CString{m_twitchFormat.c_str()});

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

	SetTimer(1, 50, nullptr);
	SetTimer(2, 500, nullptr);	// 
	SetTimer(3, 100, nullptr);	// Play Info

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
		m_spotifyAnalyzer->HandleHook();
	}
	else if (nIDEvent == 3) {
		if (m_saveTwitchInfo) {
			auto metaData = m_spotifyAnalyzer->GetMetaData();
			if (metaData != nullptr && m_spotifyAnalyzer->HasFocus() && m_spotifyAnalyzer->IsSpotifyRunning() && !m_spotifyAnalyzer->IsAdPlaying()) {
				TwitchInfo twitchInfo;
				twitchInfo.m_artist = metaData->m_artistName;
				twitchInfo.m_track = metaData->m_trackName;
				twitchInfo.m_album = metaData->m_albumName;
				twitchInfo.m_length = metaData->m_trackLength;
				twitchInfo.m_playPos = metaData->m_trackPos;

				std::wstring playString = FormatPlayString(m_twitchFormat, twitchInfo);

				if (playString != m_bufferedPlayString) {
					WritePlayString(playString, L"currentlyPlaying.txt");
					m_bufferedPlayString = playString;
				}
			}
			else if (m_spotifyAnalyzer->HasFocus() && m_spotifyAnalyzer->IsSpotifyRunning() && m_spotifyAnalyzer->IsAdPlaying()) {
				WritePlayString(m_adText, L"currentlyPlaying.txt");
			}
			else if (m_spotifyAnalyzer->HasFocus() && !m_spotifyAnalyzer->IsSpotifyRunning()) {
				WritePlayString(L"", L"currentlyPlaying.txt");
			}
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
}


void CSpotispyDlg::OnBnClickedRadioLowVolAds() {
	auto* volumeSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER1));
	volumeSlider->EnableWindow(true);

	m_spotifyAnalyzer->SetAdsBehavior(EAdsBehavior::LowerVolume);
}

void CSpotispyDlg::OnNMCustomdrawSlider1(NMHDR *pNMHDR, LRESULT *pResult) {
	auto* volumeSlider = static_cast<CSliderCtrl*>(GetDlgItem(IDC_SLIDER1));

	CString volPercentageText;
	volPercentageText.Format(L"%d", volumeSlider->GetPos());

	auto* volPercentageEdit = static_cast<CEdit*>(GetDlgItem(IDC_VOLPERCENTAGE));
	volPercentageEdit->SetWindowText(volPercentageText);
}

void CSpotispyDlg::OnBnClickedCheckSaveTwitchInfo() {
	auto* saveTwitchInfoCheckBtn = static_cast<CButton*>(GetDlgItem(IDC_CHECK_SAVE_TWITCHINFO));

	m_saveTwitchInfo = saveTwitchInfoCheckBtn->GetCheck() == 1;
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
}


void CSpotispyDlg::OnBnClickedButtonSaveAdText() {
	CString adText;

	GetDlgItemText(IDC_EDIT_AD_TEXT, adText);

	m_adText = std::wstring{adText};
}
