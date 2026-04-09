#include "stdafx.h"
#include "LoggedInUsers.h"

LoggedInUsers::LoggedInUsers(void)
{
//	vecUsers.resize(5);
}

LoggedInUsers::~LoggedInUsers(void)
{
	for(unsigned int i = 0; i < vecUsers.size(); i++)
    {
		free(vecUsers[i]);
    }
}

int LoggedInUsers::GetLoggedInUsers(void)
{
	WKSTA_USER_INFO_1 *pWUI = NULL;
	DWORD dwEntries = 0;
	DWORD dwEntriesRead = 0;
	DWORD dwResume = 0;
	int iNumUsers = 0;
	NET_API_STATUS nStatus;
	LPWSTR psUserName;
    int iWordCount;

	nStatus = NetWkstaUserEnum(NULL, 1, (LPBYTE*)&pWUI, MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwEntries, &dwResume);

	if((nStatus == NERR_Success) || (nStatus == ERROR_MORE_DATA))
	{
		if(pWUI == NULL)
			return -1;

		for(unsigned int i = 0; i < dwEntries; i++)
		{
            iWordCount = (wcslen((pWUI + i)->wkui1_logon_domain) + wcslen((pWUI + i)->wkui1_username) + 10);
			psUserName = (LPWSTR)malloc(iWordCount * sizeof(WCHAR));            
			if(psUserName == NULL)
				break;
			swprintf_s(psUserName, iWordCount, L"%s\\%s\n", (pWUI + i)->wkui1_logon_domain, (pWUI + i)->wkui1_username);
			vecUsers.push_back(psUserName);
			iNumUsers++;
		}
	}

	NetApiBufferFree(pWUI);

	return iNumUsers;
}
