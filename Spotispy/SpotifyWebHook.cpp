#include "stdafx.h"
#include "SpotifyWebHook.h"

#include <array>

SpotifyWebHook::SpotifyWebHook() {
	Init();
}

bool SpotifyWebHook::IsInitialized() const noexcept {
	std::lock_guard<std::mutex> lock{m_mutex};
	return m_hookInitialized;
}

bool SpotifyWebHook::IsInitializationOngoing() const noexcept {
	std::lock_guard<std::mutex> lock{m_mutex};
	return m_hookInitRunning;
}

std::wstring SpotifyWebHook::GetStatusMessage() const noexcept {
	std::lock_guard<std::mutex> lock{m_mutex};
	return m_statusMessage;
}

void SpotifyWebHook::NotifySpotifyClosed() noexcept {
	std::lock_guard<std::mutex> lock{m_mutex};
}

std::tuple<bool, std::unique_ptr<SpotifyMetaData>> SpotifyWebHook::GetMetaData() const noexcept {
	std::lock_guard<std::mutex> lock{m_mutex};
	if (!m_metaDataInitialized) {
		return std::make_tuple(false, nullptr);
	}

	// We have to return a copy because else it is not thread safe
	return std::make_tuple(true, std::make_unique<SpotifyMetaData>(m_bufferedMetaData));
}

void SpotifyWebHook::RefreshMetaData() noexcept {
	if (!IsInitialized()) {
		return;
	}

	using namespace web::http;
	using namespace web::http::client;

	{
		std::lock_guard<std::mutex> lock{m_mutex};
		if (!m_metaDataTaskFinished) {
			return;
		}
		m_metaDataTaskFinished = false;

		http_client client(m_localHost + L"/remote/status.json?oauth=" + m_oauth + L"&csrf=" + m_csrf);
		client.request(methods::GET).then([this] (http_response response) {
			if (response.status_code() == status_codes::OK) {
				auto jsonData = response.extract_json().get().as_object();

				// We do full exception safety garantuee, so we create a new object and if we successfully read all json values we move it into the buffered
				SpotifyMetaData metaData{};

				if (jsonData.find(L"version") != jsonData.end()) {
					metaData.m_spotifyVersion = static_cast<unsigned int>(jsonData.at(L"version").as_integer());
				}

				if (jsonData.find(L"client_version") != jsonData.end()) {
					metaData.m_clientVersion = jsonData.at(L"client_version").as_string();
				}

				if (jsonData.find(L"playing") != jsonData.end()) {
					metaData.m_playing = jsonData.at(L"playing").as_bool();
				}

				if (jsonData.find(L"shuffle") != jsonData.end()) {
					metaData.m_shuffle = jsonData.at(L"shuffle").as_bool();
				}

				if (jsonData.find(L"repeat") != jsonData.end()) {
					metaData.m_repeat = jsonData.at(L"repeat").as_bool();
				}

				if (jsonData.find(L"play_enabled") != jsonData.end()) {
					metaData.m_playEnabled = jsonData.at(L"play_enabled").as_bool();
				}

				if (jsonData.find(L"prev_enabled") != jsonData.end()) {
					metaData.m_prevEnabled = jsonData.at(L"prev_enabled").as_bool();
				}

				if (jsonData.find(L"next_enabled") != jsonData.end()) {
					metaData.m_nextEnabled = jsonData.at(L"next_enabled").as_bool();
					metaData.m_isAd = !metaData.m_nextEnabled;
				}

				if (jsonData.find(L"track") != jsonData.end()) {
					auto trackData = jsonData.at(L"track").as_object();
					if (trackData.find(L"artist_resource") != trackData.end()) {
						metaData.m_artistName = trackData.at(L"artist_resource").as_object().at(L"name").as_string();
					}
					if (trackData.find(L"track_resource") != trackData.end()) {
						metaData.m_trackName = trackData.at(L"track_resource").as_object().at(L"name").as_string();
					}
					if (trackData.find(L"album_resource") != trackData.end()) {
						metaData.m_albumName = trackData.at(L"album_resource").as_object().at(L"name").as_string();
					}

					if (trackData.find(L"length") != trackData.end()) {
						int length = trackData.at(L"length").as_integer();
						if (length < 0) {
							length = 0;
						}

						metaData.m_trackLength = std::chrono::seconds(length);
					}
				}

				if (jsonData.find(L"playing_position") != jsonData.end()) {
					float playPos = static_cast<float>(jsonData.at(L"playing_position").as_double());
					if (playPos < 0.f) {
						playPos = 0.f;
					}

					metaData.m_trackPos = std::chrono::milliseconds(static_cast<int>(playPos * 1000.f));
					metaData.m_initTime = std::chrono::steady_clock::now();
				}

				{
					std::lock_guard<std::mutex> lock{m_mutex};
					m_metaDataInitialized = true;
					m_bufferedMetaData = std::move(metaData);
				}
			}
		}).then([this] (pplx::task<void> task) {
			try {
				task.get();
			}
			catch (const std::exception&) {

			}

			// Indicate that task is finished, ready for a new one..
			std::lock_guard<std::mutex> lock{m_mutex};
			m_metaDataTaskFinished = true;
		});;
	}
}

void SpotifyWebHook::Init() {
	using namespace web::http;
	using namespace web::http::client;

	if (m_hookInitRunning) {
		return;
	}
	else {
		std::lock_guard<std::mutex> lock{m_mutex};
		m_hookInitRunning = true;
	}

	const std::wstring letters{L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"};

	auto randomWChar = [&letters] {
		std::random_device randD;

		return letters[randD() % letters.size()];
	};

	// Need to use braced ctor here
	std::wstring rndHostName(10, 0);

	std::generate_n(rndHostName.begin(), 10, randomWChar);

	http_client client(L"https://open.spotify.com/token");

	// Make the request and asynchronously process the response.
	client.request(methods::GET).then([this, rndHostName] (http_response response) {
		if (response.status_code() == status_codes::OK) {
			auto jsonData = response.extract_json().get();

			auto port = GetWorkingPort(rndHostName);
			m_localHost = GenerateLocalHostURL(rndHostName, port);

			if (!SetupOAuth(std::move(jsonData))) {
				throw std::exception{"Couldn't read OAuth token"};
			}
			if (!SetupCSRF()) {
				throw std::exception{"Couldn't read CSRF token"};
			}

			{
				std::lock_guard<std::mutex> lock{m_mutex};
				m_hookInitialized = true;
				m_hookInitRunning = false;
			}
		}
		else {
			std::lock_guard<std::mutex> lock{m_mutex};
			m_statusMessage = L"Couldn't establish connection to https://spotify.com";
			m_hookInitialized = false;
		}
	}).then([this] (pplx::task<void> task) {
		try {
			task.get();
		}
		catch (const std::exception& ex) {
			std::wostringstream errMsg;
			errMsg << ex.what() << std::endl;

			std::lock_guard<std::mutex> lock{m_mutex};
			m_statusMessage = std::move(errMsg.str());
			m_hookInitRunning = false;
			m_hookInitialized = false;
		}
	});
}

unsigned int SpotifyWebHook::GetWorkingPort(const std::wstring& rndHostName) const {
	using namespace web::http;
	using namespace web::http::client;

	unsigned int port = 4370;
	const unsigned int maxPort = 4379;

	std::vector<std::tuple<unsigned int, Concurrency::task<http_response>>> queries;

	for (; port != maxPort; ++port) {
		http_client client{L"https://" + rndHostName + L".spotilocal.com:" + std::to_wstring(port) + L"/simplecsrf/token.json"};
		queries.push_back(std::make_tuple(port, client.request(methods::GET)));
	}

	auto tasksRunning = [&queries] () {
		bool running = false;

		for (const auto& query : queries) {
			const auto& task = std::get<1>(query);

			if (!task.is_done()) {
				running = true;
				break;
			}
		}

		return running;
	};

	while (tasksRunning()) {
		std::this_thread::sleep_for(std::chrono::milliseconds{1});
	}

	std::vector<unsigned int> validPorts;

	for (const auto& query : queries) {
		try {
			const auto& task = std::get<1>(query);
			auto result = task.get();

			if (result.status_code() == status_codes::OK) {
				auto port = std::get<0>(query);
				validPorts.push_back(port);
			}
		}
		catch (const std::exception&) {
			// Not valid, connection err or something like that
		}
	}

	if (validPorts.size() == 0) {
		throw std::exception("Couldn't determine port for SpotifyWebHelper, maybe Spotify is not started so it gets rejected..?");
	}

	return validPorts.front();
}

std::wstring SpotifyWebHook::GenerateLocalHostURL(const std::wstring& rndHostName, unsigned int port) {
	return L"https://" + rndHostName + L".spotilocal.com:" + std::to_wstring(port);
}

bool SpotifyWebHook::SetupOAuth(web::json::value&& jsonData) {
	for (const auto& value : jsonData.as_object()) {
		auto oauth = value.second.serialize();

		oauth.erase(std::remove_if(oauth.begin(), oauth.end(), [] (const auto& letter) {
			return letter == '\"';
		}), oauth.end());

		m_oauth = std::move(oauth);

		// Kinda ugly but it is only one line, TODO?
		return true;
	}

	return false;
}

bool SpotifyWebHook::SetupCSRF() {
	using namespace web::http;
	using namespace web::http::client;

	http_client client{m_localHost + L"/simplecsrf/token.json"};

	http_request request{methods::GET};
	request.headers().add(L"Origin", L"https://open.spotify.com");

	auto response = client.request(request).get();

	if (response.status_code() != status_codes::OK) {
		return false;
	}

	auto jsonData = response.extract_json().get();

	if (!jsonData.has_field(L"token")) {
		return false;
	}

	auto val = jsonData.at(L"token").as_string();

	m_csrf = std::move(val);

	return true;
}

bool SpotifyWebHook::CheckFields(const web::json::object & jsonData) {
	return false;
}
