#pragma once

class SpotifyAnalyzer;

struct TwitchInfo {
	std::wstring m_track;
	std::wstring m_artist;
	std::wstring m_album;

	std::chrono::seconds m_length{0};
	std::chrono::milliseconds m_playPos{0};
};

void ReplaceAll(std::wstring& str, const std::wstring& from, const std::wstring& to);
std::wstring FormatPlayString(const std::wstring& formatString, const TwitchInfo& twitchInfo);
void WritePlayString(const std::wstring& playString, const std::wstring& fileName);