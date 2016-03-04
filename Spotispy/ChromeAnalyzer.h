//#pragma once
//
//#include "CommonCOM.h"
//#include "Helper.h"
//
//class ChromeAnalyzer
//{
//public:
//	ChromeAnalyzer(CWnd* mainWindow, std::shared_ptr<CommonCOM> com) noexcept;
//	~ChromeAnalyzer();
//
//	ChromeAnalyzer(const ChromeAnalyzer&) = delete;
//	ChromeAnalyzer& operator= (const ChromeAnalyzer&) = delete;
//	ChromeAnalyzer(ChromeAnalyzer&&) = delete;
//	ChromeAnalyzer& operator= (ChromeAnalyzer&&) = delete;
//
//	void Analyze();
//	void SetFocus(bool focus) noexcept;
//
//	bool HasFocus() const noexcept;
//
//	// Again, the only way I know how to access the class state via CSpotispyDlg->GetChromeAnalyzer->...
//	auto& GetDetectedSongs() noexcept;
//
//	void PushSong(YoutubeSong&& youtubeSong) noexcept;
//
//private:
//	CWnd* m_mainWindow;
//	std::shared_ptr<CommonCOM> m_com;
//
//	bool m_focus						{false};
//
//	void ScanWindows() noexcept;
//
//	std::tuple<HWINEVENTHOOK, bool> RegisterHook(HWND hwnd) noexcept;
//	void FreeHook(HWINEVENTHOOK hook) noexcept;
//
//	std::map<HWND, HWINEVENTHOOK> m_chromeHooks;
//	std::vector<YoutubeSong> m_detectedSongs;
//};
//
//BOOL CALLBACK EnumChromeWindows(HWND hwnd, LPARAM lParam);
//void CALLBACK HandleHookEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread, DWORD dwmsEventTime);