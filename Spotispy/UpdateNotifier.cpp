#include "stdafx.h"
#include "UpdateNotifier.h"
#include "resource.h"

#include "Poco/Dynamic/Var.h"
#include "Poco/URI.h"
#include "Poco/Path.h"
#include "Poco/Net/HTTPSClientSession.h"
#include "Poco/Net/HTTPRequest.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Net/NetException.h"
#include "Poco/JSON/Parser.h"

#include "JSONSaveFile.h"
#include "Helper.h"

UpdateNotifier::UpdateNotifier(std::wstring currentVerison, std::wstring currentBuildDate, CWnd* parentWindow)
	: m_currentVersion{std::move(currentVerison)}
	, m_currentBuildDate{std::move(currentBuildDate)}
	, m_parentWindow{parentWindow} {

	QueryResult();
}

void UpdateNotifier::QueryResult() {
	using namespace Poco;
	using namespace Poco::Net;
	using namespace Poco::JSON;

	{
		std::lock_guard<std::mutex> lock{m_mutex};
		m_hasData = false;
		m_updateInformation = UpdateInformation{};
	}

	TrueAsync([this] {
		try {
			URI uri;
			uri.setScheme("https");
			uri.setAuthority("raw.githubusercontent.com");
			uri.setPath("/hatur/Spotispy/master/Version/version.json");

			std::string path{uri.getPathAndQuery()};

			HTTPSClientSession session{uri.getHost(), uri.getPort(), g_sslContext};
			HTTPRequest request{HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1};
			HTTPResponse response;

			session.sendRequest(request);

			auto& is = session.receiveResponse(response);

			if (response.getStatus() != HTTPResponse::HTTP_OK) {
				throw std::exception("Couldn't reach remote update information");
			}

			std::string content{std::istream_iterator<char>{is}, {}};

			Parser parser;
			auto parseResult = parser.parse(content);

			auto jsonData = parseResult.extract<Object::Ptr>();

			auto version = jsonData->get("version").extract<std::string>();
			auto buildDate = jsonData->get("build_date").extract<std::string>();

			{
				std::lock_guard<std::mutex> lock{m_mutex};

				UnicodeConverter::toUTF16(version, m_updateInformation.m_version);
				UnicodeConverter::toUTF16(buildDate, m_updateInformation.m_buildDate);

				if (m_updateInformation.m_version != m_currentVersion) {
					m_updateInformation.m_updateResult = EUpdateResult::UpdateAvailable;
				}
				else {
					m_updateInformation.m_updateResult = EUpdateResult::AlreadyRecent;
				}
			}
		}
		catch (const NetException& ex) {
			std::lock_guard<std::mutex> lock{m_mutex};
			m_updateInformation.m_updateResult = EUpdateResult::NoConnection;
		}
		catch (const JSONException& ex) {
			std::lock_guard<std::mutex> lock{m_mutex};
			m_updateInformation.m_updateResult = EUpdateResult::InvalidData;
		}

		{
			std::lock_guard<std::mutex> lock{m_mutex};
			m_hasData = true;
		}
	});
}

bool UpdateNotifier::HasData() const noexcept {
	std::lock_guard<std::mutex> lock{m_mutex};
	return m_hasData;
}

UpdateInformation UpdateNotifier::FetchData() noexcept {
	std::lock_guard<std::mutex> lock{m_mutex};
	m_hasData = false;
	return m_updateInformation;
}

