#pragma once

#include "Poco/Net/Context.h"

/**
* Helper.h
*
* Collection of common helper funcitons
**/

static Poco::Net::Context::Ptr g_sslContext{new Poco::Net::Context(Poco::Net::Context::CLIENT_USE, "", "", "", Poco::Net::Context::VERIFY_NONE, 9, false, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH")};

// Helper struct for file output
struct SongInfo {
	std::wstring m_track;
	std::wstring m_artist;
	std::wstring m_album;

	std::chrono::seconds m_length{0};
	std::chrono::milliseconds m_playPos{0};
};

std::vector<CString> SeperateString(const CString& string, const CString& seperator);

bool MultiSearchCString(const CString& string, bool ignoreCase, std::vector<CString> searchValues);

CString GetWindowTitle(HWND hwnd);

CString GetWindowClassName(HWND hwnd);

bool IsProcessRunning(std::wstring processName, bool ignoreCase) noexcept;

template<typename Function, typename... Args>
inline std::future<typename std::result_of<Function(Args...)>::type> TrueAsync(Function&& f, Args&&... args) {
	typedef typename std::result_of<Function(Args...)>::type R;
	auto bound_task = std::bind(std::forward<Function>(f), std::forward<Args>(args)...);
	std::packaged_task<R()> task(std::move(bound_task));
	auto ret = task.get_future();
	std::thread t(std::move(task));
	t.detach();
	return ret;
}

void ReplaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to);

std::wstring FormatPlayString(const std::wstring& formatString, const SongInfo& twitchInfo);

void WritePlayString(const std::wstring& playString, const std::wstring& fileName);