#pragma once
#include "windows.h"

typedef struct _serviceinfo
{
	TCHAR szServiceName[256];
	DWORD dwProcessId;
	DWORD dwStartupFlag;
	DWORD dwServiceType;
	DWORD dwCurrentState;
	DWORD dwSystemService;
	DWORD dwTagId;
	TCHAR szDisplayName[512];
	TCHAR szStartupPath[512];
	TCHAR szDependencies[512];
	TCHAR szServiceOwner[512];
	TCHAR szLoadOrderGroup[512];
	TCHAR szDescription[512];
} SERVICE_INFO, *LPSERVICE_INFO;

class EnumServices
{
public:
	EnumServices(void);
	~EnumServices(void);

	const LPSERVICE_INFO QueryService(int service);
	const LPSERVICE_INFO QueryService(LPCTSTR service);

	int GetNumServices(void);
	int EnumerateServices(void);

private:
	int NumServices;
	LPSERVICE_INFO lpServiceInfo;

	// Queries and fill	s a local buffer of services
};
