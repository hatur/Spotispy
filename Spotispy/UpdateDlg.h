#pragma once

#include "UpdateNotifier.h"

class UpdateDlg : public CDialogEx
{
public:
	explicit UpdateDlg(UpdateInformation currentVersion, UpdateInformation availableVersion, std::wstring downloadLink, CWnd* pParent = nullptr);

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SPOTISPY_DIALOG };
#endif

protected:
	BOOL OnInitDialog() override;
	afx_msg HCURSOR OnQueryDragIcon();

private:
	DECLARE_MESSAGE_MAP()

	UpdateInformation m_currentVersion;
	UpdateInformation m_availableVersion;
	std::wstring m_downloadLink;

	HICON m_hIcon;
public:
	afx_msg void OnNMClickSyslinkDownloadUpdate(NMHDR *pNMHDR, LRESULT *pResult);
};

