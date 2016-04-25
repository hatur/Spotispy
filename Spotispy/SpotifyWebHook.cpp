#include "stdafx.h"
#include "SpotifyWebHook.h"

#include "SpotispyDlg.h"
#include "Helper.h"

#include "Poco/URI.h"
#include "Poco/Path.h"
#include "Poco/Exception.h"
#include "Poco/Net/HTTPClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"

#include "Poco/JSON/Parser.h"
#include "Poco/JSON/JSONException.h"
#include "Poco/Dynamic/Var.h"
#include "Poco/UnicodeConverter.h"

SpotifyWebHook::SpotifyWebHook(std::chrono::milliseconds refreshRate)
	: m_refreshRate{std::move(refreshRate)} {
	
	m_thread = std::make_unique<std::thread>(std::thread{[this] {
		std::mutex condMutex;
		std::unique_lock<std::mutex> condLock(condMutex);

		while (true) {
			// Signal from main thread to exit this worker
			m_condVar.wait(condLock, [this] {
				//
				if (m_exitRequested) {
					return true;
				}

				if (m_refreshRequest) {
					m_refreshRequest = false;
					return true;
				}

				return false;
			});

			if (m_exitRequested) {
				break;
			}

			bool needsInit = false;
			bool spotifyRunning = IsProcessRunning(L"spotify.exe", true);

			{
				std::lock_guard<std::mutex> lock{m_mutex};
				needsInit = !m_hookInitialized && spotifyRunning;
			}

			if (needsInit) {
				Init();
			}
			else if (spotifyRunning) {
				RefreshMetaData();
			}
			else if (!spotifyRunning && m_hookInitialized) {
				DeInit();
			}

			// Notify notifier Thread to wake us up after refreshRate
			m_notifierThreadNotification = true;
			m_notifierThreadCondVar.notify_all();
		}
	}});

	m_notifierThread = std::make_unique<std::thread>([this] {
		std::mutex notifierMutex;
		std::unique_lock<std::mutex> notifierLock{notifierMutex};

		while (true) {
			m_notifierThreadCondVar.wait(notifierLock, [this] {
				if (m_exitRequested) {
					return true;
				}

				if (m_notifierThreadNotification) {
					m_notifierThreadNotification = false;
					return true;
				}

				return false;
			});

			if (m_exitRequested) {
				break;
			}

			m_notifierThreadNotification = false;

			std::chrono::milliseconds refreshRate;

			{
				std::lock_guard<std::mutex> lock{m_mutex};
				refreshRate = m_refreshRate;
			}

			std::this_thread::sleep_for(refreshRate);
			m_refreshRequest = true;
			m_condVar.notify_all();
		}
	});
}

SpotifyWebHook::~SpotifyWebHook() {
	m_exitRequested = true;

	m_condVar.notify_all();
	m_notifierThreadCondVar.notify_all();

	m_thread->join();
	m_notifierThread->join();
}

void SpotifyWebHook::Init() {
	using namespace Poco;
	using namespace Poco::Net;

	//{
	//	std::lock_guard<std::mutex> lock{m_mutex};
	//	m_hookInitRunning = true;
	//}

	try {
		m_localHost = GenerateSpotilocalHostname();
		m_localPort = GetWorkingPort();

		m_oauth = SetupOAuth();
		m_csrf = SetupCSRF();

		{
			std::lock_guard<std::mutex> lock{m_mutex};
			m_hookInitialized = true;
		}

		// Refresh data first time
		RefreshMetaData();
	}
	catch (const std::exception& ex) {
		std::wostringstream errMsg;
		errMsg << ex.what();
		PushStatusEntry(EStatusEntryType::Error, errMsg.str());
	}
}

void SpotifyWebHook::DeInit() {
	std::lock_guard<std::mutex> lock{m_mutex};
	m_hookInitialized = false;
}

void SpotifyWebHook::RefreshMetaData() {
	using namespace Poco;
	using namespace Poco::Net;
	using namespace Poco::JSON;

	if (m_metaSession == nullptr || !m_metaSession->connected()) {
		if (m_metaSession != nullptr) {
			m_metaSession.reset();
		}

		URI uri;
		uri.setScheme("https");
		uri.setAuthority(m_localHost);
		uri.setPath("/remote/status.json");
		uri.setPort(m_localPort);
		uri.addQueryParameter("oauth", m_oauth);
		uri.addQueryParameter("csrf", m_csrf);
		m_metaPath = uri.getPathAndQuery();

		m_metaSession = std::make_unique<HTTPSClientSession>(uri.getHost(), uri.getPort(), g_sslContext);
		m_metaSession->setKeepAlive(true);
	}

	try {
		HTTPRequest request{HTTPRequest::HTTP_GET, m_metaPath, HTTPMessage::HTTP_1_1};
		HTTPResponse response;

		m_metaSession->sendRequest(request);

		auto& is = m_metaSession->receiveResponse(response);

		if (response.getStatus() != HTTPResponse::HTTP_OK) {
			throw std::exception(std::string{"Couldn't connect to local Spotify server: " + response.getReason() + ", when trying to get metadata"}.c_str());
		}

		auto contentType = response.getContentType();

		if (contentType.find("application/json") == std::string::npos) {
			throw std::exception("Not a valid JSON file when trying to get Spotify metadata");
		}

		std::string content{std::istreambuf_iterator<char>{is},{}};

		Parser parser;
		auto result = parser.parse(content);

		Object::Ptr object = result.extract<Object::Ptr>();

		// This might be Spotify to blame.. we need to inform the user he has to restart Spotify..
		auto error = object->has("error");

		if (error) {
			auto message = object->get("error").extract<Object::Ptr>()->get("message").extract<std::string>();
			m_spotifyNeedsRestart = true;
			DeInit(); // This can be because of an invalid CSRF or OAuth token, so we want a new initialization
			throw std::exception(std::string("Error accessing Spotify metadata: " + message + ", trying to re-establish connection").c_str());
		}
		else {
			m_spotifyNeedsRestart = false;
		}

		SpotifyMetaData metaData;

		if (object->has("version")) {
			metaData.m_spotifyVersion = object->get("version").extract<int>();
		}

		if (object->has("client_version")) {
			auto str = object->get("client_version").extract<std::string>();
			UnicodeConverter::toUTF16(str, metaData.m_clientVersion);
		}

		if (object->has("playing")) {
			metaData.m_playing = object->get("playing").extract<bool>();
		}

		if (object->has("shuffle")) {
			metaData.m_shuffle = object->get("shuffle").extract<bool>();
		}

		if (object->has("repeat")) {
			metaData.m_repeat = object->get("repeat").extract<bool>();
		}

		if (object->has("play_enabled")) {
			metaData.m_playEnabled = object->get("play_enabled").extract<bool>();
		}

		if (object->has("prev_enabled")) {
			metaData.m_prevEnabled = object->get("prev_enabled").extract<bool>();
		}

		if (object->has("next_enabled")) {
			metaData.m_nextEnabled = object->get("next_enabled").extract<bool>();
			metaData.m_isAd = !metaData.m_nextEnabled;
		}

		if (object->has("track")) {
			auto trackData = object->get("track").extract<Object::Ptr>();
			if (trackData->has("artist_resource")) {
				auto artist = trackData->get("artist_resource").extract<Object::Ptr>();
				if (artist->has("name")) {
					auto str = artist->get("name").extract<std::string>();
					UnicodeConverter::toUTF16(str, metaData.m_artistName);
				}
			}
			if (trackData->has("track_resource")) {
				auto track = trackData->get("track_resource").extract<Object::Ptr>();
				if (track->has("name")) {
					auto str = track->get("name").extract<std::string>();
					UnicodeConverter::toUTF16(str, metaData.m_trackName);
				}
			}
			if (trackData->has("album_resource")) {
				auto album = trackData->get("album_resource").extract<Object::Ptr>();
				if (album->has("name")) {
					auto str = album->get("name").extract<std::string>();
					UnicodeConverter::toUTF16(str, metaData.m_albumName);
				}
			}
			if (trackData->has("length")) {
				int length = trackData->get("length").extract<int>();
				if (length < 0) {
					length = 0;
				}
				metaData.m_trackLength = std::chrono::seconds(length);
			}
		}

		if (object->has("playing_position") && !metaData.m_isAd) {
			float playPos = 0.f;

			try {
				// If the player is stopped playing_position is appearently not a double, so it throws bad cast exception, we just leave it at 0.f
				playPos = static_cast<float>(object->get("playing_position").extract<double>());

				if (playPos < 0.f) {
					playPos = 0.f;
				}
			}
			catch (const Poco::Exception&) {

			}

			metaData.m_trackPos = std::chrono::milliseconds(static_cast<int>(playPos * 1000.f));
		}
		else if (object->has("playing_position") && metaData.m_isAd) {
			metaData.m_trackPos = std::chrono::milliseconds(0);
		}

		metaData.m_initTime = std::chrono::steady_clock::now();

		std::lock_guard<std::mutex> lock{m_mutex};
		m_bufferedMetaData = std::move(metaData);
		m_metaDataInitialized = true;
	}
	catch (const std::exception& ex) {
		std::wostringstream errMsg;
		errMsg << ex.what();
		PushStatusEntry(EStatusEntryType::Error, errMsg.str());

		m_metaSession.reset();

		std::lock_guard<std::mutex> lock{m_mutex};
		m_metaDataInitialized = false;
	}
}

unsigned int SpotifyWebHook::GetWorkingPort() const {
	using namespace Poco;
	using namespace Poco::Net;
	using namespace Poco::JSON;

	unsigned int port = 4371;
	const unsigned int maxPort = 4379;

	std::mutex queryMutex;
	std::vector<std::thread> threads;
	std::vector<unsigned int> validPorts;

	for (; port < maxPort; ++port) {
		threads.emplace_back([&queryMutex, &validPorts, port, this] {
			bool working = false;

			try {
				URI uri;
				uri.setScheme("https");
				uri.setAuthority(m_localHost);
				uri.setPort(port);
				uri.setPath("/simplecsrf/token.json");
				std::string path{uri.getPathAndQuery()};

				HTTPSClientSession session{uri.getHost(), uri.getPort(), g_sslContext};
				HTTPRequest request{HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1};
				request.set("Origin", "https://open.spotify.com");
				HTTPResponse response;

				session.sendRequest(request);

				if (response.getStatus() != HTTPResponse::HTTP_OK) {
					throw std::exception();
				}

				// Note: We don't check for malfunctioning ports here.. we need a better way to check
				// for them.. For example the 4370 port can work to acquire the CSRF token
				// but does not work on remote/status.json .. we can't make the call to that page
				// here so we maybe need something like a vector of the non working ports that
				// RefreshMetaData adds to and calls DeInit()? .. the problem is do we know for sure
				// that a null content type for remote/status.json means that the port is not working?
				// Maybe we just get away with starting to scan ports at 4371. Implement and test something
				// later if there are ever problems with the ports

				working = true;
			}
			catch (const std::exception&) {
				// We just swallow these
			}

			{
				if (working) {
					std::lock_guard<std::mutex> lock{queryMutex};
					validPorts.push_back(port);
				}
			}
		});
	}

	for (auto& thread : threads) {
		thread.join();
	}

	if (validPorts.size() == 0) {
		throw std::exception("Couldn't determine working port for local spotify server, SpotifyWebHelper.exe running?");
	}

	return validPorts.back();
}

std::string SpotifyWebHook::GenerateSpotilocalHostname() const {
	const std::string letters{"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};
	std::random_device randD;
	std::string hostname;

	for (unsigned int i = 0; i < 10; ++i) {
		hostname += letters[randD() % letters.size()];
	}

	return hostname + ".spotilocal.com";
}

std::string SpotifyWebHook::GenerateLocalHostURL(const std::string& rndHostName, unsigned int port) const {
	return "https://" + rndHostName + ".spotilocal.com:" + std::to_string(port);
}

std::string SpotifyWebHook::SetupOAuth() {
	using namespace Poco;
	using namespace Poco::Net;
	using namespace Poco::JSON;

	URI uri;
	uri.setScheme("https");
	uri.setAuthority("open.spotify.com");
	uri.setPath("/token");
	std::string path{uri.getPathAndQuery()};

	HTTPSClientSession session{uri.getHost(), uri.getPort(), g_sslContext};
	HTTPRequest request{HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1};
	HTTPResponse response;

	session.sendRequest(request);
	
	if (response.getStatus() != HTTPResponse::HTTP_OK) {
		throw std::exception(std::string{"Couldn't connect to hostname, reason: " + response.getReason()}.c_str());
	}

	auto& is = session.receiveResponse(response);

	auto contentType = response.getContentType();

	if (contentType.find("application/json") == std::string::npos) {
		throw std::exception("Couldn't get OAuth token, content type is no JSON");
	}

	std::string content{std::istreambuf_iterator<char>{is},{}};

	Parser parser;
	auto result = parser.parse(content);

	Object::Ptr object = result.extract<Object::Ptr>();

	auto tokenValue = object->get("t");

	// "An attempt to convert or extract from a non-initialized (empty) Var variable shall result in an exception being thrown."
	std::string oauthToken = tokenValue.extract<std::string>();

	return oauthToken;
}

std::string SpotifyWebHook::SetupCSRF() {
	using namespace Poco;
	using namespace Poco::Net;
	using namespace Poco::JSON;

	URI uri;
	uri.setScheme("https");
	uri.setAuthority(m_localHost);
	uri.setPort(m_localPort);
	uri.setPath("/simplecsrf/token.json");
	std::string path{uri.getPathAndQuery()};

	HTTPSClientSession session{uri.getHost(), uri.getPort(), g_sslContext};
	HTTPRequest request{HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1};
	request.set("Origin", "https://open.spotify.com");
	HTTPResponse response;

	session.sendRequest(request);

	auto& is = session.receiveResponse(response);

	if (response.getStatus() != HTTPResponse::HTTP_OK) {
		throw std::exception("Couldn't fetch CSRF from local Spotify server");
	}

	std::string content{std::istreambuf_iterator<char>{is},{}};

	Parser parser;
	auto result = parser.parse(content);

	Object::Ptr object = result.extract<Object::Ptr>();

	auto tokenValue = object->get("token");

	std::string csrfToken = tokenValue.extract<std::string>();

	return csrfToken;
}

void SpotifyWebHook::PushStatusEntry(EStatusEntryType statusEntryType, std::wstring statusEntry) {
	std::wstring prefix;
	switch (statusEntryType) {
		case EStatusEntryType::Debug:
			prefix = L"Debug: ";
			break;
		default:
		case EStatusEntryType::Error:
			prefix = L"Error: ";
			break;
	}

	std::lock_guard<std::mutex> lock{m_mutex};
	while (m_statusMessages.size() > m_maxQueuedStatusMessages - 1) {
		m_statusMessages.pop();
	}

	// Is this legit?
	auto message = std::move(prefix) + std::move(statusEntry);

	m_statusMessages.push(message);
}

void SpotifyWebHook::SetRefreshRate(std::chrono::milliseconds refreshRate) noexcept {
	std::lock_guard<std::mutex> lock{m_mutex};
	m_refreshRate = refreshRate;
}

bool SpotifyWebHook::IsInitialized() const noexcept {
	std::lock_guard<std::mutex> lock{m_mutex};
	return m_hookInitialized;
}

std::queue<std::wstring> SpotifyWebHook::FetchStatusMessages() {
	std::lock_guard<std::mutex> lock{m_mutex};
	auto queue = m_statusMessages;
	while (m_statusMessages.size() > 0) {
		m_statusMessages.pop();
	}
	return queue;
}

std::tuple<bool, std::unique_ptr<SpotifyMetaData>> SpotifyWebHook::GetMetaData() const {
	std::lock_guard<std::mutex> lock{m_mutex};
	if (!m_metaDataInitialized) {
		return std::make_tuple(false, nullptr);
	}

	// We have to return a copy because else it is not thread safe
	return std::make_tuple(true, std::make_unique<SpotifyMetaData>(m_bufferedMetaData));
}

bool SpotifyWebHook::DoesSpotfiyNeedARestart() const noexcept {
	return m_spotifyNeedsRestart;
}

// Maybe for later use:
// HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, 0, &folderPath);
