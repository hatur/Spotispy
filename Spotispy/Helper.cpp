#include "stdafx.h"
#include "Helper.h"

#include <TlHelp32.h>

std::vector<CString>SeperateString(const CString& string, const CString& seperator) {
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

bool MultiSearchCString(const CString& string, bool ignoreCase, std::vector<CString> searchValues) {
	if (ignoreCase) {
		// Need to make copies here..
		CString lowCaseString = string;
		lowCaseString.MakeLower();

		std::vector<CString> lowCaseSearchValues;

		for (const auto& searchValue : searchValues) {
			auto strCopy = searchValue;
			lowCaseSearchValues.push_back(std::move(strCopy.MakeLower()));
		}

		// Search
		for (const auto& searchLower : lowCaseSearchValues) {
			if (lowCaseString.Find(searchLower) != -1) {
				return true;
			}
		}

		return false;
	}
	else {
		for (const auto& searchValue : searchValues) {
			if (string.Find(searchValue) != -1) {
				return true;
			}
		}

		return false;
	}

	
}

CString GetWindowTitle(HWND hwnd) {
	CString windowTitle;

	unsigned int windowTitleLength = ::GetWindowTextLength(hwnd);

	auto windowTitleBuffer = windowTitle.GetBuffer(windowTitleLength);
	::GetWindowText(hwnd, windowTitleBuffer, windowTitleLength + 1);
	windowTitle.ReleaseBuffer();

	return windowTitle;
}

CString GetWindowClassName(HWND hwnd) {
	CString windowClassName;

	unsigned int windowClassNameLength = 256;

	auto windowTitleBuffer = windowClassName.GetBuffer(windowClassNameLength);
	::GetClassName(hwnd, windowTitleBuffer, windowClassNameLength + 1);
	windowClassName.ReleaseBuffer();

	return windowClassName;
}

bool IsProcessRunning(std::wstring processName, bool ignoreCase) noexcept {
	bool processExists = false;
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

	if (ignoreCase) {
		std::transform(processName.begin(), processName.end(), processName.begin(), ::tolower);
	}

	HANDLE processHandle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (Process32First(processHandle, &entry)) {
		while (Process32Next(processHandle, &entry)) {
			auto currentProcess = std::wstring{entry.szExeFile};
			if (ignoreCase) {
				std::transform(currentProcess.begin(), currentProcess.end(), currentProcess.begin(), ::tolower);
			}

			if (currentProcess == processName) {
				processExists = true;
				break;
			}
		}
	}

	CloseHandle(processHandle);

	return processExists;
}

void ReplaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to) {
	if (from.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::wstring::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
}

void WritePlayString(const std::wstring& playString, const std::wstring& fileName) {
	std::wofstream fileOut;

	// Facet cto (codecvt_utf8) manages lifetime !?
	std::locale loc{std::locale::classic(), new std::codecvt_utf8<wchar_t>};
	fileOut.imbue(loc);
	fileOut.open(fileName, std::ios::out);

	if (fileOut.good()) {
		fileOut << playString;
	}
	else {
		// Can't open file, need code / exception?
	}
}

std::wstring FormatPlayString(const std::wstring& formatString, const SongInfo& songInfo) {
	std::wstring outString{formatString};

	ReplaceAll(outString, L"%a", songInfo.m_artist);
	ReplaceAll(outString, L"%t", songInfo.m_track);
	ReplaceAll(outString, L"%u", songInfo.m_album);

	// Full time
	if (formatString.find(L"%f") != std::wstring::npos) {
		// Auto narrowing
		auto minutes = static_cast<int>(std::ceil(songInfo.m_length.count()) / 60.f);
		auto seconds = songInfo.m_length.count() - (minutes * 60);

		std::wstring fulltimeString;
		if (minutes < 10) {
			fulltimeString += L"0";
		}

		fulltimeString += std::to_wstring(minutes);
		fulltimeString += +L":";

		if (seconds < 10) {
			fulltimeString += L"0";
		}

		fulltimeString += std::to_wstring(seconds);

		ReplaceAll(outString, L"%f", fulltimeString);
	}

	// Play pos
	if (formatString.find(L"%c") != std::wstring::npos) {
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(songInfo.m_playPos);
		auto minVal = static_cast<int>((std::ceil(seconds.count()) / 60.f));
		auto secVal = seconds.count() - (minVal * 60);

		std::wstring playposString;
		if (minVal < 10) {
			playposString += L"0";
		}

		playposString += std::to_wstring(minVal);
		playposString += +L":";

		if (secVal < 10) {
			playposString += L"0";
		}

		playposString += std::to_wstring(secVal);

		ReplaceAll(outString, L"%c", playposString);
	}

	// Percentage
	if (formatString.find(L"%p") != std::wstring::npos) {
		auto playpos = static_cast<float>(songInfo.m_playPos.count());
		auto length = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(songInfo.m_length).count());

		auto percentage = (playpos * 100.f) / length;

		auto absPercentage = static_cast<int>(std::round(percentage));

		ReplaceAll(outString, L"%p", std::to_wstring(absPercentage) + L"%");
	}

	return outString;
}