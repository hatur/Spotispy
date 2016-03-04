
// Spotispy.h: Hauptheaderdatei für die PROJECT_NAME-Anwendung
//

#pragma once

#ifndef __AFXWIN_H__
	#error "'stdafx.h' vor dieser Datei für PCH einschließen"
#endif

#include "resource.h"		// Hauptsymbole


// CSpotispyApp:
// Siehe Spotispy.cpp für die Implementierung dieser Klasse
//

class CSpotispyApp : public CWinApp
{
public:
	CSpotispyApp();

// Überschreibungen
public:
	virtual BOOL InitInstance();

// Implementierung

	DECLARE_MESSAGE_MAP()
};

extern CSpotispyApp theApp;