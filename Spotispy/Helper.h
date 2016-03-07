#pragma once

/**
* Collection of static Helper functions
**/
class Helper
{
public:
	template<typename T>
	static std::vector<T> SeperateString(const T& string, const T& seperator);

	template<>
	static std::vector<CString> SeperateString(const CString& string, const CString& seperator);

	// @ ignoreCase = Convert everything to lower and search then (ignoring case)
	static bool MultiSearchCString(const CString& string, bool ignoreCase, std::vector<CString> searchValues);

	static CString GetWindowTitle(HWND hwnd);
	static CString GetWindowClassName(HWND hwnd);

	// Potentially expensive if many processes are running..
	static bool IsProcessRunning(std::wstring processName, bool ignoreCase) noexcept;

private:
	Helper() = delete;
};

template<typename T>
inline std::vector<T> Helper::SeperateString(const T& string, const T& seperator) {
	static_assert(false, "Not yet implemented!");

	return std::vector<T>();
}

template<>
inline std::vector<CString> Helper::SeperateString(const CString& string, const CString& seperator) {
	std::vector<CString> subStrs;

	int lastPos = 0;
	bool hasNext = true;

	do {
		int nextPos = string.Find(seperator, lastPos);

		if (nextPos == -1) {
			subStrs.emplace_back(string.Mid(lastPos));
			break;
		}

		subStrs.emplace_back(string.Mid(lastPos, nextPos - lastPos));

		lastPos = nextPos + seperator.GetLength();

		hasNext = string.Find(seperator, lastPos) != -1;

		if (!hasNext) {
			// Add rest
			auto rest = string.Mid(lastPos);
			if (rest.GetLength() != 0) {
				subStrs.push_back(std::move(rest));
			}
		}
	} while (hasNext);

	return subStrs;
}

std::wstring ConvertStrToWStr(const std::string& str);
std::string ConvertWStrToStr(const std::wstring& wstr);

template<typename Function, typename... Args>
inline std::future<typename std::result_of<Function(Args...)>::type> trueAsync(Function&& f, Args&&... args) {
	typedef typename std::result_of<Function(Args...)>::type R;
	auto bound_task = std::bind(std::forward<Function>(f), std::forward<Args>(args)...);
	std::packaged_task<R()> task(std::move(bound_task));
	auto ret = task.get_future();
	std::thread t(std::move(task));
	t.detach();
	return ret;
}

struct YoutubeSong {
	CString m_name;
	HWND m_hwndStarted;
	bool m_started;
};

inline bool operator== (const YoutubeSong& lhs, const YoutubeSong& rhs) {
	if (lhs.m_name != rhs.m_name) {
		return false;
	}

	return true;
}

inline bool operator!= (const YoutubeSong& lhs, const YoutubeSong& rhs) {
	return !(lhs == rhs);
}
