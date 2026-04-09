#pragma once

#include "stdafx.h"
#include <vector>

using namespace std;

typedef struct _STRUCT_Apps {
	TCHAR szAppName[MAX_PATH];
	TCHAR szPath[MAX_PATH];
	TCHAR szVersion[MAX_PATH];
	TCHAR szPublisher[MAX_PATH];
} APPS, *PAPPS;

class AppList
{
public:
	AppList(void);
	~AppList(void);

	int GetApps(void);

	vector<PAPPS> vecApp;
};
