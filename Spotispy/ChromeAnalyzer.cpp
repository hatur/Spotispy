#include "stdafx.h"

//#include "ChromeAnalyzer.h"
//
//#include "resource.h"
//#include "TwitchIO.h"
//#include "SpotispyDlg.h"
//
//ChromeAnalyzer::ChromeAnalyzer(CWnd* mainWindow, std::shared_ptr<CommonCOM> com) noexcept
//	: m_mainWindow{mainWindow}
//	, m_com{std::move(com)} {
//
//}
//
//ChromeAnalyzer::~ChromeAnalyzer() {
//	for (const auto& entry : m_chromeHooks) {
//		UnhookWinEvent(entry.second);
//	}
//}
//
//void ChromeAnalyzer::Analyze() {
//	ScanWindows();
//}
//
//void ChromeAnalyzer::SetFocus(bool focus) noexcept {
//	m_focus = focus;
//}
//
//bool ChromeAnalyzer::HasFocus() const noexcept {
//	return m_focus;
//}
//
//auto& ChromeAnalyzer::GetDetectedSongs() noexcept {
//	return m_detectedSongs;
//}
//
//void ChromeAnalyzer::PushSong(YoutubeSong&& youtubeSong) noexcept {
//	bool process = false;
//
//	if (m_detectedSongs.size() == 0) {
//		process = true;
//	}
//	else {
//		if (m_detectedSongs.back() != youtubeSong) {
//			process = true;
//		}
//	}
//
//	if (process) {
//		m_detectedSongs.push_back(std::move(youtubeSong));
//
//		const auto& youtubeSong2 = m_detectedSongs.back();
//
//		auto* ytList = static_cast<CListBox*>(m_mainWindow->GetDlgItem(IDC_LIST_YOUTUBE_FILTER));
//		ytList->InsertString(0, youtubeSong2.m_name);
//
//		m_mainWindow->SetDlgItemText(IDC_EDIT_SONG_BROWSER, youtubeSong2.m_name);
//	}
//}
//
//void ChromeAnalyzer::ScanWindows() noexcept {
//	std::vector<HWND> allWindows;
//	std::vector<HWND> newWindows;
//
//	EnumWindows(EnumChromeWindows, reinterpret_cast<LPARAM>(&allWindows));
//
//	// Buffer new windows
//	for (const auto& window : allWindows) {
//		if (m_chromeHooks.find(window) == m_chromeHooks.end()) {
//			newWindows.push_back(window);
//		}
//	}
//
//	std::map<HWND, HWINEVENTHOOK> bufferHooks;
//	std::vector<HWINEVENTHOOK> goneHooks;
//
//	// Find still existing hooks
//	for (const auto& window : m_chromeHooks) {
//		if (std::find(allWindows.begin(), allWindows.end(), window.first) != allWindows.end()) {
//			bufferHooks.insert(window);
//		}
//		else {
//			goneHooks.push_back(window.second);
//		}
//	}
//
//	for (const auto& hook : goneHooks) {
//		FreeHook(hook);
//	}
//
//	for (const auto& window : newWindows) {
//		HWINEVENTHOOK hook;
//		bool success;
//
//		std::tie(hook, success) = RegisterHook(window);
//
//		if (success) {
//			bufferHooks.insert(std::make_pair(window, hook));
//		}
//	}
//
//	m_chromeHooks = std::move(bufferHooks);
//
//	int i = m_chromeHooks.size() + 22;
//}
//
//std::tuple<HWINEVENTHOOK, bool> ChromeAnalyzer::RegisterHook(HWND hwnd) noexcept {
//	unsigned long processID;
//
//	GetWindowThreadProcessId(hwnd, &processID);
//
//	if (processID != 0) {
//		auto hook = SetWinEventHook(
//			EVENT_MIN, EVENT_MAX,
//			0,
//			HandleHookEvent,
//			processID,
//			0,
//			WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
//		);
//
//		return std::make_tuple(hook, true);
//	}
//
//	return std::make_tuple(HWINEVENTHOOK{}, false);
//}
//
//void ChromeAnalyzer::FreeHook(HWINEVENTHOOK hook) noexcept {
//	UnhookWinEvent(hook);
//}
//
//BOOL CALLBACK EnumChromeWindows(HWND hwnd, LPARAM lParam) {
//	auto winVec = reinterpret_cast<std::vector<HWND>*>(lParam);
//
//	auto windowClassName = Helper::GetWindowClassName(hwnd);
//
//	if (windowClassName == L"Chrome_WidgetWin_1"/*|| windowClassName == L"Chrome_WidgetWin_0"*/) {
//		winVec->push_back(hwnd);
//	}
//
//	return TRUE;
//}
//
//void CALLBACK HandleHookEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime) {
//	IAccessible* pAcc = nullptr;
//	VARIANT varChild;
//	HRESULT hr = AccessibleObjectFromEvent(hwnd, idObject, idChild, &pAcc, &varChild);
//
//	if ((hr == S_OK) && (pAcc != nullptr)) {
//		auto* tempList = static_cast<CListBox*>(AfxGetApp()->m_pMainWnd->GetDlgItem(IDC_LIST_TEMPEVENTS));
//		auto* ytList = static_cast<CListBox*>(AfxGetApp()->m_pMainWnd->GetDlgItem(IDC_LIST_YOUTUBE_FILTER));
//
//		// This is fucking ugly but I don't think we have a choice..
//		auto dlgSingleton = static_cast<CSpotispyDlg*>(AfxGetApp()->m_pMainWnd);
//
//		BSTR bstrName;
//		BSTR bstrValue;
//		BSTR bstrDesc;
//
//		pAcc->get_accName(varChild, &bstrName);
//		pAcc->get_accValue(varChild, &bstrValue);
//		pAcc->get_accDefaultAction(varChild, &bstrDesc);
//
//		auto name = Helper::GetWindowClassName(hwnd);
//
//		CString eventId;
//		eventId.Format(L"%u", event);
//
//		CString objectID;
//		objectID.Format(L"%ld", idObject);
//
//		auto insertString = bstrName + CString{", Value: "} + bstrValue + CString{", OID: "} + objectID + CString{L", ID: "} + eventId;
//		bool skipEntry = false;
//
//		auto numEntries = static_cast<unsigned int>(tempList->GetCount());
//
//		//for (unsigned int i = 0; i < 1 && i < numEntries; ++i) {
//		//	CString content;
//		//	tempList->GetText(i, content);
//
//		//	if (content == insertString) {
//		//		skipEntry = true;
//		//		break;
//		//	}
//		//}
//
//		if (!skipEntry) {
//			//tempList->InsertString(0, insertString);
//		}
//
//		auto cstrName = CString{bstrName};
//		int ytStringPos = cstrName.Find(L"- YouTube -");
//
//		if (ytStringPos != -1) {
//			auto cutName = cstrName.Left(ytStringPos);
//			dlgSingleton->GetChromeAnalyzer()->PushSong(YoutubeSong{cutName, hwnd, false});
//		}
//
//		SysFreeString(bstrName);
//		SysFreeString(bstrValue);
//		SysFreeString(bstrDesc);
//		pAcc->Release();
//	}
//}
