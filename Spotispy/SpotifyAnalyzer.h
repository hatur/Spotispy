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

	// Not used atm, determines if this context (spotify) has the focus, maybe change later
	void SetFocus(bool focus);
	void SetAdsBehavior(EAdsBehavior newBehavior);

	// Called on program exit, unmutes spotify and sets volume to saved
	void RestoreDefaults();

	// Expensive(?), enumerates all processes
	static bool IsSpotifyRunning() noexcept;

	// Expensive(?), enumerates all processes
	static bool IsWebHelperRunning() noexcept;

	bool HasFocus() const noexcept;
	bool IsAdPlaying() const noexcept;
	std::wstring GetCurrentlyPlayingTrack() const;
	const SpotifyMetaData* GetMetaData() const noexcept;

private:
	bool InitSpotifyAudioContext();
	void OnAdStatusChanged();

	CWnd* m_mainWindow;
	std::shared_ptr<CommonCOM> m_com;

	bool m_focus {false};
	EAdsBehavior m_adsBehavior {EAdsBehavior::Mute};
	bool m_spotifyAudioInitialized {false};
	ISimpleAudioVolume* m_spotifyAudio {nullptr};
	float m_savedVolume {1.0f};
	std::unique_ptr<SpotifyWebHook> m_webHook {nullptr};
	std::unique_ptr<SpotifyMetaData> m_metaData	{nullptr};
};

