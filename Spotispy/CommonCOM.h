#pragma once

/**
* CommonCOM.h
*
* Used as RAII-style class that holds windows comm open for different subclasses
* We used it before in the Chrome tool, maybe we will need it later
**/
class CommonCOM
{
public:
	CommonCOM();
	~CommonCOM();

	bool IsInitialized() const noexcept;

private:
	bool m_initialized {false};
};

