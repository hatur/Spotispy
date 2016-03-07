#pragma once

#include "Poco/Net/Context.h"

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

enum class EStatusEntryType {
	// We only need those two
	Debug,
	Error
};

/**
* We put this into an extra class because it works async and we can seperate the implementation nicely this way
**/
class SpotifyWebHook {
public:
	// @param refreshRate - The time between each call to Init() or RefreshMetaData(), the actual time between the refreshes is the time the request takes + refreshRate
	SpotifyWebHook(std::chrono::milliseconds refreshRate);
	~SpotifyWebHook();

	SpotifyWebHook(const SpotifyWebHook&) = delete;
	SpotifyWebHook& operator= (const SpotifyWebHook&) = delete;
	SpotifyWebHook(SpotifyWebHook&&) = delete;
	SpotifyWebHook& operator= (SpotifyWebHook&&) = delete;

	// @param refreshRate - The new refreshRate in seconds, please note that the new refreshRate will first kick in after the next refresh (ongoing wait) is finished
	void SetRefreshRate(std::chrono::milliseconds refreshRate) noexcept;

	// Returns whether the hook is initialized and ready to operate
	bool IsInitialized() const noexcept;

	// This returns the current statusmessages and deletes them from the internal vetor
	std::queue<std::wstring> FetchStatusMessages() noexcept;

	// Returns the gathered Data
	std::tuple<bool, std::unique_ptr<SpotifyMetaData>> GetMetaData() const noexcept;

private:
	unsigned int GetWorkingPort() const;
	std::string GenerateSpotilocalHostname();
	std::string GenerateLocalHostURL(const std::string& rndHostName, unsigned int port);

	std::string SetupOAuth();
	std::string SetupCSRF();

	// Init() and RefreshMetaData() are automatically called through the worker thread, never call them manually

	void Init();
	void DeInit() noexcept;
	void RefreshMetaData() noexcept;

	// This function first checks if there already are too many entries and if not pushes it to m_statusMessages
	void PushStatusEntry(EStatusEntryType statusEntryType, std::wstring statusEntry);

	//bool IsWebHelperRunning() const noexcept;
	//bool InitWebHelper() const noexcept;

	std::chrono::milliseconds m_refreshRate;
	std::atomic<bool> m_refreshRequest			{true};

	const Poco::Net::Context::Ptr m_sslContext;

	std::unique_ptr<std::thread> m_thread		{nullptr};
	std::atomic<bool> m_exitRequested			{false};
	//bool m_hookInitRunning						{false};
	bool m_hookInitialized						{false};
	std::string m_localHost						{};
	unsigned int m_localPort					{(std::numeric_limits<unsigned int>::max)()};
	std::string m_oauth							{};
	std::string m_csrf							{};
	//bool m_metaDataTaskRunning					{false};
	std::atomic<bool> m_spotifyNeedsRestart{false};
	bool m_metaDataInitialized					{false};
	SpotifyMetaData m_bufferedMetaData			{};

	unsigned int m_maxQueuedStatusMessages		{20};
	std::queue<std::wstring> m_statusMessages;
	mutable std::mutex m_mutex;

	std::condition_variable m_condVar;
	mutable std::mutex m_condMutex;
};
