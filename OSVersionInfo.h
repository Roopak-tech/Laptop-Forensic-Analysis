#pragma once

#include "stdafx.h"

class OSVersionInfo
{
public:
	OSVersionInfo(void);
	~OSVersionInfo(void);

	TCHAR sOSName[MAX_PATH];
	TCHAR sOSShortName[MAX_PATH];
	TCHAR sServicePack[MAX_PATH];
	DWORD dwMajorVersion;
	DWORD dwMinorVersion;
	DWORD dwBuild;
	DWORD dwServicePackMajor;
	DWORD dwServicePackMinor;
	BOOL  bIs64Bit;

	void GetOSVersion(void);
	BOOL CheckIs64BitOS(void);
};
