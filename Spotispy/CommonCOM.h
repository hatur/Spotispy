#pragma once

class CommonCOM
{
public:
	CommonCOM();
	~CommonCOM();

	bool IsInitialized() const noexcept;

private:
	bool m_initialized {false};
};

