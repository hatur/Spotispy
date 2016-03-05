#include "stdafx.h"
#include "SpotifyWebHook.h"

#include <array>

#include "SpotispyDlg.h"
#include "Helper.h"

SpotifyWebHook::SpotifyWebHook() {
	Init();
}

void SpotifyWebHook::Init() {
	using namespace web::http;
	using namespace web::http::client;

	{
		std::lock_guard<std::mutex> lock{m_mutex};
		if (m_hookInitRunning) {
			return;
		}
		else {
			m_hookInitRunning = true;
		}
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
			bool isJson = false;

			auto headers = response.headers();
			auto it = headers.find(L"Content-Type");
			if (it != headers.end()) {
				auto data = it->second.data();

				if (std::wstring{data}.find(L"application/json") != std::wstring::npos) {
					isJson = true;
				}
			}

			if (!isJson) {
				return;
			}

			auto jsonData = response.extract_json().get();

			auto port = GetWorkingPort(rndHostName);

			{
				std::lock_guard<std::mutex> lock{m_mutex};
				m_localHost = GenerateLocalHostURL(rndHostName, port);
			}

			if (!SetupOAuth(std::move(jsonData))) {
				throw std::exception{"Couldn't read OAuth token"};
			}
			if (!SetupCSRF()) {
				throw std::exception{"Couldn't read CSRF token"};
			}

			{
				std::lock_guard<std::mutex> lock{m_mutex};
				m_hookInitialized = true;
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
			m_hookInitialized = false;
		}

		{
			std::lock_guard<std::mutex> lock{m_mutex};
			m_hookInitRunning = false;
		}
	});
}

void SpotifyWebHook::Uninitialize() {
	std::lock_guard<std::mutex> lock{m_mutex};
	m_hookInitialized = false;
	m_metaDataInitialized = false;
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
		client.request(methods::GET).then([this] (http_response response) -> pplx::task<web::json::value> {
			if (response.status_code() == status_codes::OK) {
				bool isJson = false;

				auto headers = response.headers();
				auto it = headers.find(L"Content-Type");
				if (it != headers.end()) {
					auto data = it->second.data();

					if (std::wstring{data}.find(L"application/json") != std::wstring::npos) {
						isJson = true;
					}
				}

				if (!isJson) {
					std::lock_guard<std::mutex> lock{m_mutex};
					m_metaDataInitialized = false;
					m_metaDataStatus = EMetaDataStatus::ErrorRetrieving;
					return pplx::task_from_result(web::json::value());
				}

				return response.extract_json();
			}
			else {
				return pplx::task_from_result(web::json::value());
			}
		}).then([this] (pplx::task<web::json::value> previousTask) {
			try {
				const auto& jsonDataTask = previousTask.get();
				auto jsonData = jsonDataTask.as_object();

				if (jsonData.size() == 0) {
					std::lock_guard<std::mutex> lock{m_mutex};
					m_metaDataInitialized = false;
					m_metaDataStatus = EMetaDataStatus::ErrorRetrieving;
					m_metaDataTaskFinished = true;
				}

				if (jsonData.find(L"error") != jsonData.end()) {
					// The page we got through the WebHelper is describing an error, we uninitialize the connection and
					// set the status message to error to be able to view it in the App, yet we don't exactly determine
					// the error, the source and possible solutions for it, should we do so? (TODO)
					Uninitialize();

					{
						std::lock_guard<std::mutex> lock{m_mutex};
						auto type = m_statusMessage = jsonData.at(L"error").as_object().at(L"type").as_string();
						if (type == L"4102") {
							// 4102 is invalid OAuth token, which makes no sense to me since it's always the same
							// This should be ill-described and we just tell the user to restart Spotify because
							// this is the only known way to solve it?
							m_statusMessage = std::wstring{L"Error validating OAuth token, try to restart Spotify"};
						}
						else {
							m_statusMessage = jsonData.at(L"error").as_object().at(L"message").as_string();
						}
						m_metaDataInitialized = false;
						m_metaDataStatus = EMetaDataStatus::ErrorRetrieving;
						m_metaDataTaskFinished = true;
					}

					return;
				}

				// We always need to return this, even if fields are missing (which they are, when ads are playing)
				// So we can either check the fields and return what we have or we check only the fields that are present
				// while Ads are playing.. TODO: Optimization
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
					m_metaDataStatus = EMetaDataStatus::Success;
					m_bufferedMetaData = std::move(metaData);
				}
			}
			catch (const std::exception& ex) {
				std::wostringstream errMsg;
				errMsg << ex.what() << std::endl;

				std::lock_guard<std::mutex> lock{m_mutex};
				m_metaDataInitialized = false;
				m_metaDataStatus = EMetaDataStatus::ErrorRetrieving;
			}

			std::lock_guard<std::mutex> lock{m_mutex};
			m_metaDataTaskFinished = true;
		});
	}
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

		{
			std::lock_guard<std::mutex> lock{m_mutex};
			m_oauth = std::move(oauth);
		}

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

	{
		std::lock_guard<std::mutex> lock{m_mutex};
		m_csrf = std::move(val);
	}

	return true;
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

std::tuple<EMetaDataStatus, std::unique_ptr<SpotifyMetaData>> SpotifyWebHook::GetMetaData() const noexcept {
	std::lock_guard<std::mutex> lock{m_mutex};
	if (m_metaDataStatus == EMetaDataStatus::ErrorRetrieving) {
		return std::make_tuple(m_metaDataStatus, nullptr);
	}
	else if (!m_metaDataInitialized) {
		return std::make_tuple(EMetaDataStatus::NoData, nullptr);
	}

	// We have to return a copy because else it is not thread safe
	return std::make_tuple(EMetaDataStatus::Success, std::make_unique<SpotifyMetaData>(m_bufferedMetaData));
}

//bool SpotifyWebHook::IsWebHelperRunning() const noexcept {
//	return Helper::IsProcessRunning(L"spotifywebhelper.exe", true);
//}
//
//bool SpotifyWebHook::InitWebHelper() const noexcept {
//	LPWSTR folderPath;
//	HRESULT hr = SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, 0, &folderPath);
//
//	if (hr != S_OK) {
//		MessageBox(0, L"Error, couldn't determine %APPDATA% folder, this program needs Windows Vista or later!", L"Error", MB_ICONERROR);
//		std::terminate();
//	}
//
//	// TODO: Do we still need to manually start the web helper?
//}
