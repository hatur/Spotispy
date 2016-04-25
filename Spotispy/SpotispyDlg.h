#pragma once

#include "SpotifyAnalyzer.h"
#include "JSONSaveFile.h"

const std::wstring SPOTISPY_VERSION = L"v0.4.3";
const std::wstring SPOTISPY_BUILD_DATE = L"14.03.2016";

class UpdateNotifier;

class CSpotispyDlg : public CDialogEx
{
public:
	explicit CSpotispyDlg(CWnd* pParent = nullptr);
	~CSpotispyDlg();

	CSpotispyDlg(const CSpotispyDlg&) = default;
	CSpotispyDlg& operator= (const CSpotispyDlg&) = default;
	CSpotispyDlg(CSpotispyDlg&&) = default;
	CSpotispyDlg& operator= (CSpotispyDlg&&) = default;

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SPOTISPY_DIALOG };
#endif

	void MessageTerminate(const CString& quitMessage) const noexcept;

	bool GetSaveTwitchInfo() const noexcept;

protected:
	void DoDataExchange(CDataExchange* pDX) override;

private:
	void OnTimer(UINT nIDEvent);
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRadioMuteAds();
	afx_msg void OnBnClickedRadioLowVolAds();
	afx_msg void OnNMCustomdrawSlider1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCheckSaveTwitchInfo();
	afx_msg void OnBnClickedButtonSaveTwitchFormat();
	afx_msg void OnBnClickedButtonSaveAdText();
	afx_msg void OnBnClickedButtonSaveStreamInfo();
	afx_msg void OnNMClickSyslinkCheckUpdate(NMHDR *pNMHDR, LRESULT *pResult);

	BOOL OnInitDialog() override;
	afx_msg void OnPaint();
	afx_msg void OnClose();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

	HICON m_hIcon;

	std::unique_ptr<SpotifyAnalyzer> m_spotifyAnalyzer {nullptr};
	std::unique_ptr<UpdateNotifier> m_updateNotifier {nullptr};

	JSONSaveFile m_dlgSave{"dlgPrefs.json"};

	bool m_saveTwitchInfo;

	std::wstring m_twitchFilePath;

	std::wstring m_twitchFormat;
	std::wstring m_adText;

	std::wstring m_bufferedPlayString;

};

BOOL CALLBACK EnumWindowsProc_GetWindowTitle(HWND hwnd, LPARAM lParam);
BOOL CALLBACK EnumWindowsProc_GetClassName(HWND hwnd, LPARAM lParam);