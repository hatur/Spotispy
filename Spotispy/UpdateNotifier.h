#pragma once

enum class EUpdateResult {
	NotInitiated,
	NoConnection,
	InvalidData,
	AlreadyRecent,
	UpdateAvailable
};

struct UpdateInformation {
	EUpdateResult m_updateResult {EUpdateResult::NotInitiated};
	std::wstring m_version;
	std::wstring m_buildDate;
};

class UpdateNotifier
{
public:
	UpdateNotifier(std::wstring currentVerison, std::wstring currentBuildDate, CWnd* parentWindow);

	// Called through the custom ctor, only call this is you default construct and use the setter later
	void QueryResult();

	void OpenDownloadPage();

	bool HasData() const noexcept;
	UpdateInformation FetchData() noexcept;

private:
	const std::wstring m_currentVersion;
	const std::wstring m_currentBuildDate;

	CWnd* m_parentWindow;

	bool m_hasData {false};
	UpdateInformation m_updateInformation;
	mutable std::mutex m_mutex;
};

