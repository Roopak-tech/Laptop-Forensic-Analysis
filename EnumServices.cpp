#include "EnumServices.h"
#include "tchar.h"

#pragma comment(lib, "advapi32.lib")

EnumServices::EnumServices(void)
{
	NumServices	  = 0;
	lpServiceInfo = NULL;
	
}

EnumServices::~EnumServices(void)
{
	if(lpServiceInfo)
		HeapFree(GetProcessHeap(), 0, lpServiceInfo);
}

// Queries and fills a local buffer of services
int EnumServices::EnumerateServices(XMLNode &root)
{
	SC_HANDLE hSCManager = NULL;
	SC_HANDLE hService   = NULL;
	
	DWORD nBytesNeeded	  = 0;
	DWORD nBytesRemaining = 0;
	DWORD nEnumSize		  = 0;
	DWORD nNumServices	  = 0;

	LPENUM_SERVICE_STATUS_PROCESS lpEnum   = NULL;
	LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;
	LPSERVICE_DESCRIPTION lpServiceDesc = NULL;
	
	// Open the Service Control Manager
	hSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, GENERIC_READ);

	if(!hSCManager)
		return 0;

	// Query to see how much room we need for lpEnum
	EnumServicesStatusEx(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_DRIVER | SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &nBytesNeeded, &nNumServices, 0, NULL);

	nEnumSize = nBytesNeeded;

	// Allocation lpEnum
	lpEnum = (LPENUM_SERVICE_STATUS_PROCESS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nEnumSize);

	nBytesNeeded = 0;

	// Query all of the services
	EnumServicesStatusEx(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_DRIVER | SERVICE_WIN32, SERVICE_STATE_ALL, (LPBYTE)lpEnum, nEnumSize, &nBytesNeeded, &nNumServices, 0, NULL);

	// nNumServices is populated on the SECOND call that actually enumerates the services.
	NumServices = nNumServices;

	// Allocate the appropriate amount of lpServices
	lpServiceInfo = (LPSERVICE_INFO)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, NumServices * sizeof(SERVICE_INFO));

	for(DWORD i = 0; i < nNumServices; i++)
	{
		// Open each service to extract data
		hService = OpenService(hSCManager, lpEnum[i].lpServiceName, GENERIC_READ);

		// Validate the handle
		if(hService)
		{
			nBytesNeeded = 0;
			nBytesRemaining = 0;

			QueryServiceConfig(hService, NULL, 0, &nBytesNeeded);

			lpServiceConfig = (LPQUERY_SERVICE_CONFIG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nBytesNeeded);

			// Validate lpServiceConfig

			if(lpServiceConfig)
			{
				QueryServiceConfig(hService, lpServiceConfig, nBytesNeeded, &nBytesRemaining);

				lpServiceInfo[i].dwCurrentState = lpEnum[i].ServiceStatusProcess.dwCurrentState;
				lpServiceInfo[i].dwProcessId = lpEnum[i].ServiceStatusProcess.dwProcessId;
				lpServiceInfo[i].dwServiceType = lpServiceConfig->dwServiceType;
				lpServiceInfo[i].dwStartupFlag = lpServiceConfig->dwStartType;
				lpServiceInfo[i].dwSystemService = lpEnum[i].ServiceStatusProcess.dwServiceFlags;
				lpServiceInfo[i].dwTagId = lpEnum[i].ServiceStatusProcess.dwServiceFlags;
				
				_tcscpy_s(lpServiceInfo[i].szDependencies, 512, lpServiceConfig->lpDependencies);
				_tcscpy_s(lpServiceInfo[i].szDisplayName, 512, lpEnum[i].lpDisplayName);
				_tcscpy_s(lpServiceInfo[i].szServiceName, 512, lpEnum[i].lpServiceName);
				_tcscpy_s(lpServiceInfo[i].szServiceOwner, 512, lpServiceConfig->lpServiceStartName);
				_tcscpy_s(lpServiceInfo[i].szStartupPath, 512, lpServiceConfig->lpBinaryPathName);
				_tcscpy_s(lpServiceInfo[i].szLoadOrderGroup, 512, lpServiceConfig->lpLoadOrderGroup);
			}

			QueryServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, NULL, 0, &nBytesNeeded);
			lpServiceDesc = (LPSERVICE_DESCRIPTION) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nBytesNeeded);
			if (lpServiceDesc != NULL) {
				// make sure to check that there is actually a description!
				if (QueryServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, (LPBYTE) lpServiceDesc, 
					nBytesNeeded, &nBytesRemaining) && lpServiceDesc->lpDescription != NULL) {
						_tcscpy_s(lpServiceInfo[i].szDescription, 1024, lpServiceDesc->lpDescription);
				}
			}

			HeapFree(GetProcessHeap(), 0, lpServiceConfig);
			lpServiceConfig = NULL;			

			CloseServiceHandle(hService);
			hService = NULL;
		}
		else
		{
			// Skip it for now...
		}
	}

	HeapFree(GetProcessHeap(), 0, lpEnum);

	CloseServiceHandle(hSCManager);

	return 1;
}

int EnumServices::GetNumServices(void)
{
	return NumServices;
}

const LPSERVICE_INFO EnumServices::QueryService(int service)
{
	// Validate that we've retrieved the service info
	if(!lpServiceInfo)
		return NULL;

	// Validate that service lies in our range
	if(service <= NumServices)
		return &lpServiceInfo[service];
	else
		return NULL;
}

const LPSERVICE_INFO EnumServices::QueryService(LPCTSTR service)
{
	int i = 0;

	if(!lpServiceInfo)
		return NULL;

	while(_tcscmp(lpServiceInfo[i].szServiceName, service) && i < NumServices)
		i++;
	
	if(i < NumServices)
		return &lpServiceInfo[i];
	else
		return NULL;
}
