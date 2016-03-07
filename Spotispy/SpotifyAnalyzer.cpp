#include "stdafx.h"
#include "SpotifyAnalyzer.h"

#include "resource.h"
#include "Helper.h"

SpotifyAnalyzer::SpotifyAnalyzer(CWnd* mainWindow, std::shared_ptr<CommonCOM> com) noexcept
	: m_mainWindow{mainWindow}
	, m_com{std::move(com)} {

	InitSpotifyAudioContext();

	// Fail safe inits..
	auto* muteAdsRadio = static_cast<CButton*>(m_mainWindow->GetDlgItem(IDC_RADIO_MUTEADS));

	if (muteAdsRadio->GetCheck() == 1) {
		m_adsBehavior = EAdsBehavior::Mute;
	}
	else {
		m_adsBehavior = EAdsBehavior::LowerVolume;
	}

	m_webHook = std::make_unique<SpotifyWebHook>(std::chrono::milliseconds(500));
}

SpotifyAnalyzer::~SpotifyAnalyzer() {
	if (m_spotifyAudio != nullptr) {
		m_spotifyAudio->Release();
	}
}

void SpotifyAnalyzer::Analyze() {
	bool spotifyRunning = IsSpotifyRunning();

	if (!m_spotifyAudioInitialized) {
		if (spotifyRunning) {
			if (!InitSpotifyAudioContext()) {
				// This sometimes happens, Spotify is for some reason started but the the audio context is not yet initialized,
				// it will be there if music starts playing
				m_mainWindow->SetDlgItemText(IDC_EDIT_SONG_SPOTIFY, L"Spotify started, start playing a song..");
				return;
			}
		}
	}

	// De-Init audio
	if (!spotifyRunning && m_spotifyAudioInitialized) {
		if (m_spotifyAudio != nullptr) {
			m_spotifyAudio->Release();
			m_spotifyAudio = nullptr;
		}
		m_spotifyAudioInitialized = false;

		m_mainWindow->SetDlgItemText(IDC_EDIT_SONG_SPOTIFY, L"Spotify not started!");
	}

	// Save the volume .. in case it was changed
	if (m_spotifyAudioInitialized && !IsAdPlaying()) {
		float audioVolume;
		m_spotifyAudio->GetMasterVolume(&audioVolume);

		m_savedVolume = audioVolume;
	}

	bool metaDataRetrievalSuccess = false;

	if (m_webHook->IsInitialized()) {
		std::tie(metaDataRetrievalSuccess, m_metaData) = m_webHook->GetMetaData();
	}

	auto statusMessages = m_webHook->FetchStatusMessages();

	if (m_webHook->DoesSpotfiyNeedARestart()) {
		// We can't do shit about this..
		m_mainWindow->SetDlgItemText(IDC_EDIT_SONG_SPOTIFY, L"Problem with OAuth, restart Spotfiy..");
	}
	else if (metaDataRetrievalSuccess) {
		// Only updates the EDIT to let us see the song in the application
		if (m_metaData->m_isAd) {
			m_mainWindow->SetDlgItemText(IDC_EDIT_SONG_SPOTIFY, L"Waiting for ad to finish..");
		}
		else {
			std::wstring fullTitle{m_metaData->m_artistName + L" - " + m_metaData->m_trackName + L" (" + m_metaData->m_albumName + L")"};
			m_mainWindow->SetDlgItemText(IDC_EDIT_SONG_SPOTIFY, CString{fullTitle.c_str()});
		}

		bool adPlaying = IsAdPlaying();

		if (m_spotifyAudioInitialized && !m_muted && adPlaying) {
			OnAdStatusChanged();
		}
		else if (m_spotifyAudioInitialized && m_muted && !adPlaying) {
			OnAdStatusChanged();
		}

		m_muted = adPlaying;
	}
	else if (!metaDataRetrievalSuccess) {
		if (statusMessages.size() > 0) {
			m_mainWindow->SetDlgItemText(IDC_EDIT_SONG_SPOTIFY, CString{statusMessages.back().c_str()});
		}
	}
}

void SpotifyAnalyzer::RestoreDefaults() noexcept {
	if (m_spotifyAudio != nullptr) {
		m_spotifyAudio->SetMute(0, 0);
		m_spotifyAudio->SetMasterVolume(m_savedVolume, 0);
	}
}

void SpotifyAnalyzer::SetFocus(bool focus) noexcept {
	m_focus = focus;
}

void SpotifyAnalyzer::SetAdsBehavior(EAdsBehavior newBehavior) noexcept {
	m_adsBehavior = newBehavior;

	// Reset mode
	RestoreDefaults();
	OnAdStatusChanged();
}

bool SpotifyAnalyzer::InitSpotifyAudioContext() noexcept {
	HRESULT hr;

	IMMDeviceEnumerator* deviceEnumerator = nullptr;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), reinterpret_cast<LPVOID*>(&deviceEnumerator));
	if (hr != S_OK) { return false; }

	IMMDevice* defaultDevice = nullptr;
	hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
	if (hr != S_OK) { return false; }

	IAudioEndpointVolume* endpointVolume = nullptr;
	hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<LPVOID*>(&endpointVolume));
	if (hr != S_OK) { return false; }

	IAudioSessionManager2* sessionManager = nullptr;
	hr = defaultDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, nullptr, reinterpret_cast<LPVOID*>(&sessionManager));
	if (hr != S_OK) { return false; }

	IAudioSessionEnumerator* sessionEnumerator = nullptr;
	hr = sessionManager->GetSessionEnumerator(&sessionEnumerator);
	if (hr != S_OK) { return false; }

	int sessionCount = 0;
	hr = sessionEnumerator->GetCount(&sessionCount);
	if (hr != S_OK) { return false; }

	IAudioSessionControl* sessionControl = nullptr;
	IAudioSessionControl2* sessionControl2 = nullptr;

	bool foundSpotify = false;

	for (int i = 0; i < sessionCount; ++i) {
		if (foundSpotify) {
			break;
		}

		hr = sessionEnumerator->GetSession(i, &sessionControl);
		if (hr != S_OK) { return false; }

		hr = sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), reinterpret_cast<LPVOID*>(&sessionControl2));
		if (hr != S_OK) { return false; }

		LPWSTR retString;
		hr = sessionControl2->GetSessionInstanceIdentifier(&retString);
		if (hr != S_OK) { return false; }

		CString convString{retString};

		if (convString.Find(L"Spotify") != -1) {
			GUID guid;
			hr = IIDFromString(retString, (LPCLSID)(&guid));

			ISimpleAudioVolume* simpleAudioVolume = nullptr;
			hr = sessionControl->QueryInterface(__uuidof(ISimpleAudioVolume), reinterpret_cast<LPVOID*>(&simpleAudioVolume));

			m_spotifyAudio = simpleAudioVolume;
			m_spotifyAudioInitialized = true;

			m_spotifyAudio->GetMasterVolume(&m_savedVolume);

			foundSpotify = true;
		}

		sessionControl2->Release();
		sessionControl->Release();
	}

	sessionEnumerator->Release();
	sessionManager->Release();
	endpointVolume->Release();
	defaultDevice->Release();
	deviceEnumerator->Release();

	return foundSpotify;
}

void SpotifyAnalyzer::OnAdStatusChanged() noexcept {
	if (m_spotifyAudio == nullptr) {
		return;
	}

	if (IsAdPlaying()) {
		if (m_adsBehavior == EAdsBehavior::Mute) {
			m_spotifyAudio->SetMute(1, 0);
		}
		else if (m_adsBehavior == EAdsBehavior::LowerVolume) {
			auto* volumeSlider = static_cast<CSliderCtrl*>(m_mainWindow->GetDlgItem(IDC_SLIDER1));
			float newVolume = static_cast<float>(volumeSlider->GetPos()) / 100.f;

			m_spotifyAudio->GetMasterVolume(&m_savedVolume);
			m_spotifyAudio->SetMasterVolume(newVolume, 0);
		}
	}
	else {
		if (m_adsBehavior == EAdsBehavior::Mute) {
			m_spotifyAudio->SetMute(0, 0);
		}
		else if (m_adsBehavior == EAdsBehavior::LowerVolume) {
			m_spotifyAudio->SetMasterVolume(m_savedVolume, 0);
		}
	}
}

bool SpotifyAnalyzer::HasFocus() const noexcept {
	return m_focus;
}

bool SpotifyAnalyzer::IsAdPlaying() const noexcept {
	if (m_metaData == nullptr) {
		return false;
	}
	else {
		return m_metaData->m_isAd;
	}
}

bool SpotifyAnalyzer::IsSpotifyRunning() noexcept {
	return Helper::IsProcessRunning(L"spotify.exe", true);
}

bool SpotifyAnalyzer::IsWebHelperRunning() noexcept {
	return Helper::IsProcessRunning(L"spotifywebhelper.exe", true);
}

std::wstring SpotifyAnalyzer::GetCurrentlyPlayingTrack() const noexcept {
	CString dialogTemp;
	m_mainWindow->GetDlgItemText(IDC_EDIT_SONG_SPOTIFY, dialogTemp);

	return std::wstring{dialogTemp};
}

const SpotifyMetaData* SpotifyAnalyzer::GetMetaData() const noexcept {
	return m_metaData.get();
}