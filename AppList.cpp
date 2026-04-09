#include "StdAfx.h"
#include "AppList.h"

AppList::AppList(void)
{
}

AppList::~AppList(void)
{
	// Clear out the string array since we're done with it.
	for(unsigned int i = 0; i < vecApp.size(); i++)
	{
		free(vecApp[i]);
	}
}

int AppList::GetApps(void)
{
	HKEY hkPath;
	HKEY hkKey;
	unsigned int iIndex = 0;
	unsigned int iNumKeys = 0;
	unsigned int iMaxKeyLen = 0;
	unsigned int iKeyLen = 0;
	unsigned int iValueLen = 0;
	LONG lResult = 0;
	LPTSTR pKeyName = NULL;
	LPTSTR pAppName = NULL;
	TCHAR szRegPath[MAX_PATH] = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall");
	PAPPS app;

	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegPath, 0, KEY_READ, &hkPath) != ERROR_SUCCESS)
	{
		return 0;
	}

	lResult = RegQueryInfoKey(hkPath, NULL, NULL, NULL, (DWORD*)&iNumKeys, (DWORD*)&iMaxKeyLen, NULL, NULL, NULL, NULL, NULL, NULL);

	if(lResult != ERROR_SUCCESS)
	{
		return 0;
	}

	iKeyLen = iMaxKeyLen + 2;
	pKeyName = (LPTSTR)malloc(iKeyLen * sizeof(TCHAR));

	while((lResult = RegEnumKeyEx(hkPath, iIndex, pKeyName, (DWORD*)&iKeyLen, NULL, NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS)
	{
		// Skip this entry as there's no app name
		if((lResult = RegOpenKeyEx(hkPath, pKeyName, NULL, KEY_READ, &hkKey)) != ERROR_SUCCESS)
		{
			// The key doesn't exist?  No point in continuing.
			RegCloseKey(hkPath);
			free(pKeyName);
			return 0;
		}

		app = (PAPPS)malloc(sizeof(APPS));
		memset(app, 0, sizeof(APPS));

		// Here's to hoping none of these are more than MAX_PATH long
		iValueLen = MAX_PATH;
		lResult = RegQueryValueEx(hkKey, _T("DisplayName"), NULL, NULL, (LPBYTE)app->szAppName, (DWORD*)&iValueLen);

		if(lResult == ERROR_SUCCESS)
		{		
			iValueLen = MAX_PATH;
			lResult = RegQueryValueEx(hkKey, _T("DisplayVersion"), NULL, NULL, (LPBYTE)app->szVersion, (DWORD*)&iValueLen);
			
			iValueLen = MAX_PATH;		
			lResult = RegQueryValueEx(hkKey, _T("InstallLocation"), NULL, NULL, (LPBYTE)app->szPath, (DWORD*)&iValueLen);

			iValueLen = MAX_PATH;		
			lResult = RegQueryValueEx(hkKey, _T("Publisher"), NULL, NULL, (LPBYTE)app->szPublisher, (DWORD*)&iValueLen);

			vecApp.push_back(app);
		}
		else
		{
			free(app);
		}

		memset(pKeyName, 0, iMaxKeyLen * sizeof(TCHAR));
		iKeyLen = iMaxKeyLen + 2;
		iIndex++;
	}

	RegCloseKey(hkPath);
	free(pKeyName);

	return iIndex;
}