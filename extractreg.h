#pragma once


#define _UNICODE
#define UNICODE 

#ifndef _EXTRACT_REG_H_
#define _EXTRACT_REG_H_

#include "windows.h"
#include "HashedXML.h"
#include "map"
#include "vector"

using namespace std;
#pragma comment(lib, "advapi32.lib")

//What are the keys and subkeys that we want to extract





/*SubKeys Under HKCU*/

//Explorer SubKeys of interest

/*vector<LPTSTR> HKCUSubKeyNames = {
								_T("Software\Microsoft\Windows\CurrentVersion\Explorer\ComDlg32\OpenSaveMRU")
								_T("Software\Microsoft\Windows\CurrentVersion\Explorer\ComDlg32\LastVisitedMRU"),
								_T("Software\Microsoft\Windows\CurrentVersion\Explorer\RecentDocs"),
								_T("Software\Microsoft\Windows\CurrentVersion\Explorer\RunMRU"),
								_T("Software\Microsoft\Windows\CurrentVersion\Explorer\MountPoints2\CPC\Volume"),
								_T("Software\Microsoft\Search Assistant\ACMru"),
								_T("Software\Microsoft\Command Processor"),
								_T("Software\Microsoft\Windows\CurrentVersion\Explorer\UserAssist")
};
*/

/*
DWORD EnumerateRegistry(DWORD, HashedXMLFile &);
DWORD EnumerateSubKeys(HKEY, LPCTSTR, DWORD, LPCTSTR, HashedXMLFile &);
DWORD EnumerateValues(HKEY key, LPCTSTR subkey, DWORD indents, LPCTSTR keypath, HashedXMLFile &, bool bIncludeFT);

void Indent(DWORD, FILE*);
LPTSTR EscapeString(LPCTSTR);
DWORD ExpandString(LPTSTR*, LPDWORD, LPCTSTR);
*/
#endif // _EXTRACT_REG_H_