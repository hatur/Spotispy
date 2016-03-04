#include "stdafx.h"
#include "CommonCOM.h"

CommonCOM::CommonCOM() {
	if (CoInitialize(0) == S_OK) {
		m_initialized = true;
	}
}

CommonCOM::~CommonCOM() {
	CoUninitialize();
}

bool CommonCOM::IsInitialized() const noexcept {
	return m_initialized;
}
