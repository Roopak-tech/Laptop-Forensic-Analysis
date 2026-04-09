#include "stdafx.h"
#include "OSVersionInfo.h"
#include "WSTLog.h"

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
LPFN_ISWOW64PROCESS fnIsWow64Process;

OSVersionInfo::OSVersionInfo(void)
{
	memset(sOSName, 0, MAX_PATH * sizeof(TCHAR));
	memset(sOSShortName, 0, MAX_PATH * sizeof(TCHAR));
	memset(sServicePack, 0, MAX_PATH * sizeof(TCHAR));
	
	dwMajorVersion = 0;
	dwMinorVersion = 0;
	dwBuild = 0;
	dwServicePackMajor = 0;
	dwServicePackMinor = 0;
	bIs64Bit = FALSE;

	GetOSVersion();
}

OSVersionInfo::~OSVersionInfo(void)
{
}

void OSVersionInfo::GetOSVersion(void)
{
	TCHAR sTempName[MAX_PATH] = _T("");
	TCHAR sBits[MAX_PATH] = _T("");
	OSVERSIONINFOEX osvex;

	memset(&osvex, 0, sizeof(OSVERSIONINFOEX));
	osvex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((LPOSVERSIONINFO)&osvex);

	dwMajorVersion = osvex.dwMajorVersion;
	dwMinorVersion = osvex.dwMinorVersion;
	dwBuild = osvex.dwBuildNumber;
	dwServicePackMajor = osvex.wServicePackMajor;
	dwServicePackMinor = osvex.wServicePackMinor;
	bIs64Bit = CheckIs64BitOS();

	if(bIs64Bit)
    {
        AppLog.Write("64-bit Operating System", WSLMSG_PROF);
		_tcscpy_s(sBits, MAX_PATH, _T("x64"));
    }
	else
    {
        AppLog.Write("32-bit Operating System", WSLMSG_PROF);
		_tcscpy_s(sBits, MAX_PATH, _T("x86"));
    }

	_tcscpy_s(sServicePack, MAX_PATH, osvex.szCSDVersion);

	if(osvex.wProductType = VER_NT_SERVER)
	{
		switch(dwMajorVersion)
		{
		case 4:
			_tcscpy_s(sTempName, MAX_PATH, _T("Windows NT Version 4.0"));
			break;
		case 5:
			if(dwMinorVersion == 0)
				_tcscpy_s(sTempName, MAX_PATH, _T("Windows Server 2000"));
			if(dwMinorVersion == 2)
			{
				if(GetSystemMetrics(89))
					_tcscpy_s(sTempName, MAX_PATH, _T("Windows Server 2003 R2"));
				else
					_tcscpy_s(sTempName, MAX_PATH, _T("Windows Server 2003"));
			}
			break;
		case 6:
			_tcscpy_s(sTempName, MAX_PATH, _T("Windows Server 2008"));
			break;
		default:
			_tcscpy_s(sTempName, MAX_PATH, _T("Unknown Windows Version"));
			break;
		}

		if(osvex.wSuiteMask = VER_SUITE_ENTERPRISE)
			_tcscat_s(sTempName, MAX_PATH, _T(" Enterprise"));
	}

	if(osvex.wProductType & VER_NT_WORKSTATION)
	{
		switch(dwMajorVersion)
		{
		case 4:
			_tcscpy_s(sTempName, MAX_PATH, _T("Windows NT Workstation"));
			break;
		case 5:
			if(dwMinorVersion == 0)
				_tcscpy_s(sTempName, MAX_PATH, _T("Windows 2000 Professional"));
			if(dwMinorVersion == 1)
			{
				if(osvex.wSuiteMask & VER_SUITE_PERSONAL)
					_tcscpy_s(sTempName, MAX_PATH, _T("Windows XP Home Edition"));
				else
					_tcscpy_s(sTempName, MAX_PATH, _T("Windows XP Professional"));
			}
			if(dwMinorVersion == 2)
			{
				_tcscpy_s(sTempName, MAX_PATH, _T("Windows XP Professional x64 Edition"));
			}
			break;
		case 6:
			if(osvex.wSuiteMask & VER_SUITE_PERSONAL)
				_tcscpy_s(sTempName, MAX_PATH, _T("Windows Vista Home"));
			else
				_tcscpy_s(sTempName, MAX_PATH, _T("Windows Vista"));
			break;
		default:
			_tcscpy_s(sTempName, MAX_PATH, _T("Unknown Windows Version"));
			break;
		}
	}

	if(dwServicePackMajor)
	{
		_stprintf_s(sOSShortName, MAX_PATH, _T("%s SP%u (%s)"), sTempName, dwServicePackMajor, sBits);
		_stprintf_s(sOSName, MAX_PATH, _T("%s SP%u (%s) [Version %u.%u Build %u]"), sTempName, dwServicePackMajor, sBits, dwMajorVersion, dwMinorVersion, dwBuild);
	}
	else
	{
		_stprintf_s(sOSShortName, MAX_PATH, _T("%s (%s)"), sTempName, sBits);
		_stprintf_s(sOSName, MAX_PATH, _T("%s (%s) [Version %u.%u Build %u]"), sTempName, sBits, dwMajorVersion, dwMinorVersion, dwBuild);
	}
}

// Ripped right out of MSDN
BOOL OSVersionInfo::CheckIs64BitOS(void)
{	
    BOOL bIsWow64 = FALSE;

    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
	AppLog.Write("IsWow64Process API", WSLMSG_PROF);
  
    if(NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
        {
			return FALSE;
        }
    }

    return bIsWow64;
}