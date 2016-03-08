#pragma once

#include "Poco/Dynamic/Var.h"
#include "Poco/JSON/Object.h"

/**
* JSONSaveFile class
*
* The top container in the save file should be named "data"
**/
class JSONSaveFile
{
public:
	JSONSaveFile() noexcept;
	explicit JSONSaveFile(std::string fileName) noexcept;

	~JSONSaveFile();

	JSONSaveFile(const JSONSaveFile&) = default;
	JSONSaveFile& operator= (const JSONSaveFile&) = default;
	JSONSaveFile(JSONSaveFile&&) = default;
	JSONSaveFile& operator= (JSONSaveFile&&) = default;

	// Set's the file name, discards all saved data and inits data with new file if file exists
	void SetFileName(std::string fileName) noexcept;

	// Inits class data with file data
	bool LoadFromFile() noexcept;

	bool SaveToFile() const noexcept;

	bool Has(const std::string& key) const noexcept;

	template<typename T>
	T Get(const std::string& key) const;

	template<typename T>
	T GetOrSaveDefault(const std::string& key, T defaultVal) noexcept;

	template <typename T>
	bool Insert(const std::string& key, T value) noexcept;

private:
	// Clears class data
	void ClearData() noexcept;

	std::string m_fileName;

	Poco::JSON::Object::Ptr m_data;
};

template<typename T>
inline T JSONSaveFile::Get(const std::string& key) const {
	return m_data->get(key).extract<T>();
}

template<typename T>
inline T JSONSaveFile::GetOrSaveDefault(const std::string& key, T defaultVal) noexcept {
	if (Has(key)) {
		try {
			auto val = Get<T>(key);
			return val;
		}
		catch (const std::exception&) {
			Insert(key, defaultVal);
			return defaultVal;
		}
	}
	else {
		Insert(key, defaultVal);
		return defaultVal;
	}
}

template<typename T>
inline bool JSONSaveFile::Insert(const std::string& key, T value) noexcept {
	try {
		auto var = Poco::Dynamic::Var(value);

		// If var are different types it does not update the val for .. reasons .. so we remove it first, maybe there is a better solution..
		if (Has(key)) {
			m_data->remove(key);
		}

		m_data->set(key, var);
		return true;
	}
	catch (const std::exception&) {
		return false;
	}
}