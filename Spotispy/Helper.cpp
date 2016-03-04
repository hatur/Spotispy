#include "stdafx.h"
#include "Helper.h"

#include <TlHelp32.h>

bool Helper::MultiSearchCString(const CString& string, bool ignoreCase, std::vector<CString> searchValues) {
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

CString Helper::GetWindowTitle(HWND hwnd) {
	CString windowTitle;

	unsigned int windowTitleLength = ::GetWindowTextLength(hwnd);

	auto windowTitleBuffer = windowTitle.GetBuffer(windowTitleLength);
	::GetWindowText(hwnd, windowTitleBuffer, windowTitleLength + 1);
	windowTitle.ReleaseBuffer();

	return windowTitle;
}

CString Helper::GetWindowClassName(HWND hwnd) {
	CString windowClassName;

	unsigned int windowClassNameLength = 256;

	auto windowTitleBuffer = windowClassName.GetBuffer(windowClassNameLength);
	::GetClassName(hwnd, windowTitleBuffer, windowClassNameLength + 1);
	windowClassName.ReleaseBuffer();

	return windowClassName;
}

bool Helper::IsProcessRunning(std::wstring processName, bool ignoreCase) noexcept {
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