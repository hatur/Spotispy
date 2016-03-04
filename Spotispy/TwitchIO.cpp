#include "stdafx.h"
#include "TwitchIO.h"

#include <fstream>
#include <locale>
#include <codecvt>

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

std::wstring FormatPlayString(const std::wstring& formatString, const TwitchInfo & twitchInfo) {
	std::wstring outString{formatString};

	ReplaceAll(outString, L"%a", twitchInfo.m_artist);
	ReplaceAll(outString, L"%t", twitchInfo.m_track);
	ReplaceAll(outString, L"%u", twitchInfo.m_album);

	// Full time
	if (formatString.find(L"%f") != std::wstring::npos) {
		// Auto narrowing
		auto minutes = static_cast<int>(std::ceil(twitchInfo.m_length.count()) / 60.f);
		auto seconds = twitchInfo.m_length.count() - (minutes * 60);

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
		auto seconds = std::chrono::duration_cast<std::chrono::seconds>(twitchInfo.m_playPos);
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
		auto playpos = static_cast<float>(twitchInfo.m_playPos.count());
		auto length = static_cast<float>(std::chrono::duration_cast<std::chrono::milliseconds>(twitchInfo.m_length).count());

		auto percentage = (playpos * 100.f) / length;

		auto absPercentage = static_cast<int>(std::round(percentage));

		ReplaceAll(outString, L"%p", std::to_wstring(absPercentage) + L"%");
	}

	return outString;
}
