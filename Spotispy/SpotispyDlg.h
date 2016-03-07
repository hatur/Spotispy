#pragma once

#include "SpotifyAnalyzer.h"

const std::wstring SPOTISPY_VERSION = L"v0.3";

class CSpotispyDlg : public CDialogEx
{
public:
	explicit CSpotispyDlg(CWnd* pParent = NULL);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SPOTISPY_DIALOG };
#endif

	void MessageTerminate(const CString& quitMessage) const noexcept;

	bool GetSaveTwitchInfo() const noexcept;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV-Unterstützung

	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedRadioMuteAds();
	afx_msg void OnBnClickedRadioLowVolAds();
	afx_msg void OnNMCustomdrawSlider1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedCheckSaveTwitchInfo();
	afx_msg void OnBnClickedButtonSaveTwitchFormat();
	afx_msg void OnBnClickedButtonSaveAdText();

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg void OnClose();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()

private:
	void OnTimer(UINT nIDEvent);

	HICON m_hIcon;

	std::unique_ptr<SpotifyAnalyzer> m_spotifyAnalyzer;

	bool m_saveTwitchInfo;

	std::wstring m_twitchFormat{L"Currently playing: %a - %t [%c / %f]"};
	std::wstring m_adText{L"..waiting.."};

	std::wstring m_bufferedPlayString;
};

BOOL CALLBACK EnumWindowsProc_GetWindowTitle(HWND hwnd, LPARAM lParam);
BOOL CALLBACK EnumWindowsProc_GetClassName(HWND hwnd, LPARAM lParam);