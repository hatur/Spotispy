#include "stdafx.h"
#include "UpdateDlg.h"
#include "afxdialogex.h"
#include "resource.h"

UpdateDlg::UpdateDlg(UpdateInformation currentVersion, UpdateInformation availableVersion, std::wstring downloadLink, CWnd* pParent)
	: CDialogEx{IDD_UPDATE_DIALOG, pParent}
	, m_currentVersion{std::move(currentVersion)}
	, m_availableVersion{std::move(availableVersion)}
	, m_downloadLink{std::move(downloadLink)} {

	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);
}

BOOL UpdateDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	auto& currentVersionST = dynamic_cast<CStatic&>(*GetDlgItem(IDC_CURRENT_VERSION));
	currentVersionST.SetWindowText(CString{m_currentVersion.m_version.c_str()});

	auto& availableVersionST = dynamic_cast<CStatic&>(*GetDlgItem(IDC_AVAILABLE_VERSION));
	availableVersionST.SetWindowText(CString{m_availableVersion.m_version.c_str()});

	return TRUE;
}

HCURSOR UpdateDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

BEGIN_MESSAGE_MAP(UpdateDlg, CDialogEx)
	ON_NOTIFY(NM_CLICK, IDC_SYSLINK_DOWNLOAD_UPDATE, &UpdateDlg::OnNMClickSyslinkDownloadUpdate)
END_MESSAGE_MAP()



void UpdateDlg::OnNMClickSyslinkDownloadUpdate(NMHDR *pNMHDR, LRESULT *pResult) {
	ShellExecute(0, L"open", m_downloadLink.c_str(), 0, 0, SW_SHOWNORMAL);
}
