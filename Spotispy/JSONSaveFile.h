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
	explicit JSONSaveFile(std::string fileName);

	~JSONSaveFile();

	JSONSaveFile(const JSONSaveFile&) = default;
	JSONSaveFile& operator= (const JSONSaveFile&) = default;
	JSONSaveFile(JSONSaveFile&&) = default;
	JSONSaveFile& operator= (JSONSaveFile&&) = default;

	// Set's the file name, discards all saved data and inits data with new file if file exists
	void SetFileName(std::string fileName);

	// Inits class data with file data
	bool LoadFromFile();

	bool SaveToFile() const;

	bool Has(const std::string& key) const noexcept;

	template<typename T>
	T Get(const std::string& key) const;

	template<typename T>
	T GetOrSaveDefault(const std::string& key, T defaultVal);

	template <typename T>
	bool Insert(const std::string& key, T value);

private:
	// Clears class data
	void ClearData();

	std::string m_fileName;

	Poco::JSON::Object::Ptr m_data {nullptr};
};

template<typename T>
inline T JSONSaveFile::Get(const std::string& key) const {
	return m_data->get(key).extract<T>();
}

template<typename T>
inline T JSONSaveFile::GetOrSaveDefault(const std::string& key, T defaultVal) {
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
inline bool JSONSaveFile::Insert(const std::string& key, T value) {
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