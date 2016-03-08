#include "stdafx.h"
#include "JSONSaveFile.h"

#include "Poco/JSON/Parser.h"
#include "Poco/UnicodeConverter.h"

JSONSaveFile::JSONSaveFile() noexcept {}

JSONSaveFile::JSONSaveFile(std::string fileName) noexcept
	: m_fileName{std::move(fileName)}
	, m_data{nullptr} {
	
}

JSONSaveFile::~JSONSaveFile() {
	SaveToFile();
}

void JSONSaveFile::SetFileName(std::string fileName) noexcept {
	m_fileName = std::move(fileName);
}

bool JSONSaveFile::LoadFromFile() noexcept {
	using namespace Poco;
	using namespace Poco::JSON;

	std::wifstream fileStream;
	std::locale loc{std::locale::classic(), new std::codecvt_utf8<wchar_t>};
	fileStream.imbue(loc);
	fileStream.open(m_fileName);

	m_data.assign(nullptr);

	Parser parser;

	if (!fileStream.good()) {
		m_data = parser.parse("{ }").extract<Object::Ptr>();
		return false;
	}

	std::wstring wcontent{std::basic_istream<wchar_t>::_Iter(fileStream), {}};

	std::string content;

	Poco::UnicodeConverter::toUTF8(wcontent, content);

	try {
		auto parseResult = parser.parse(content);
		m_data = parseResult.extract<Object::Ptr>();
	}
	catch (const std::exception&) {
		// If we can't read the file we just load an empty file it since everything has to declare default anyways..
		m_data = parser.parse("{ }").extract<Object::Ptr>();
	}

	return true;
}

bool JSONSaveFile::SaveToFile() const noexcept {
	std::wofstream fileOut;
	std::locale loc{std::locale::classic(),  new std::codecvt_utf8<wchar_t>};
	fileOut.imbue(loc);
	fileOut.open(m_fileName);

	if (!fileOut.good()) {
		return false;
	}

	std::stringstream ss;
	m_data->stringify(ss, 1);

	std::wstring outContent;
	Poco::UnicodeConverter::toUTF16(ss.str(), outContent);

	fileOut << outContent;

	return true;
}

bool JSONSaveFile::Has(const std::string& key) const noexcept {
	try {
		auto val = m_data->has(key);
		return val;
	}
	catch (const std::exception&) {
		return false;
	}
}

