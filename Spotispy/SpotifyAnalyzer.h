#pragma once

#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>

#include "CommonCOM.h"
#include "SpotifyWebHook.h"

enum class EAdsBehavior {
	Mute,
	LowerVolume
};

class SpotifyAnalyzer
{
public:
	SpotifyAnalyzer(CWnd* mainWindow, std::shared_ptr<CommonCOM> com) noexcept;
	~SpotifyAnalyzer();

	SpotifyAnalyzer(const SpotifyAnalyzer&) = default;
	SpotifyAnalyzer& operator= (const SpotifyAnalyzer&) = default;
	SpotifyAnalyzer(SpotifyAnalyzer&&) = default;
	SpotifyAnalyzer& operator= (SpotifyAnalyzer&&) = default;

	// Processes spotify track data and handles sound mute / volume lowering
	void Analyze();

	// Trys to fetch spoify data or initialize the hook if the hook is not established yet
	void HandleHook() noexcept;

	// Not used atm, determines if this context (spotify) has the focus, maybe change later
	void SetFocus(bool focus) noexcept;
	void SetAdsBehavior(EAdsBehavior newBehavior) noexcept;

	// Called on program exit, unmutes spotify and sets volume to saved
	void RestoreDefaults() noexcept;

	// Expensive(?), enumerates all processes
	static bool IsSpotifyRunning() noexcept;

	// Expensive(?), enumerates all processes
	static bool IsWebHelperRunning() noexcept;

	bool HasFocus() const noexcept;
	bool IsAdPlaying() const noexcept;
	std::wstring GetCurrentlyPlayingTrack() const noexcept;
	const SpotifyMetaData* GetMetaData() const noexcept;

private:
	bool InitSpotifyAudioContext() noexcept;
	void OnAdStatusChanged() noexcept;

	CWnd* m_mainWindow;
	std::shared_ptr<CommonCOM> m_com;

	bool m_focus								{false};
	EAdsBehavior m_adsBehavior					{EAdsBehavior::Mute};
	bool m_spotifyAudioInitialized				{false};
	ISimpleAudioVolume* m_spotifyAudio			{nullptr};
	float m_savedVolume							{1.0f};
	bool m_muted								{false};
	std::unique_ptr<SpotifyWebHook> m_webHook	{nullptr};
	std::unique_ptr<SpotifyMetaData> m_metaData	{nullptr};
};

