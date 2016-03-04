#pragma once

#include "cpprest/http_client.h"
#include "cpprest/filestream.h"

template<typename T>
struct SpotifyQueryResult {
	bool m_success{false};
	T m_data;
};

struct SpotifyMetaData {
	unsigned int m_spotifyVersion;
	std::wstring m_clientVersion;
	
	bool m_playing;
	bool m_shuffle;
	bool m_repeat;

	bool m_playEnabled;
	bool m_prevEnabled;
	bool m_nextEnabled;

	bool m_isAd;

	std::wstring m_trackName;
	std::wstring m_artistName;
	std::wstring m_albumName;

	std::chrono::seconds m_trackLength;
	std::chrono::milliseconds m_trackPos;
	std::chrono::steady_clock::time_point m_initTime;
};

/**
* We put this into an extra class because it works async and we can seperate the implementation nicely this way
**/
class SpotifyWebHook {
public:
	// Initializing the Hook may block, so use a thread to manage it, it's internally protected by a mutex
	SpotifyWebHook();

	using httpTask = concurrency::task<web::http::http_response>;

	bool IsInitialized() const noexcept;
	bool IsInitializationOngoing() const noexcept;
	std::wstring GetStatusMessage() const noexcept;

	void NotifySpotifyClosed() noexcept;

	// Trys to refresh the buffered meta data with actual values, starts a task, does not block
	void RefreshMetaData() noexcept;

	// Returns the gathered Data
	std::tuple<bool, std::unique_ptr<SpotifyMetaData>> GetMetaData() const noexcept;

	// Note: You don't need to call this, it is automatically called through the constructor, only call this if you want to re-initialize the web context
	void Init();

private:
	unsigned int GetWorkingPort(const std::wstring& rndHostName) const;
	std::wstring GenerateLocalHostURL(const std::wstring& rndHostName, unsigned int port);

	bool SetupOAuth(web::json::value&& jsonData);
	bool SetupCSRF();

	bool CheckFields(const web::json::object& jsonData);

	bool m_hookInitRunning						{false};
	bool m_hookInitialized						{false};
	std::wstring m_statusMessage				{};

	std::wstring m_localHost					{};

	std::wstring m_oauth						{};
	std::wstring m_csrf							{};

	std::unique_ptr<httpTask> m_metaDataTask	{nullptr};
	bool m_metaDataTaskFinished					{true};

	bool m_metaDataInitialized					{false};
	SpotifyMetaData m_bufferedMetaData			{};

	mutable std::mutex m_mutex;
};
