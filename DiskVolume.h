// DiskVolume.h -
// Container for a single Volume GUID and associated enumerated information.
// diswko 1/29/08 - Initial version
// dsiwko 1/30/08 - Reworked the internals to make it slightly more in line with the
//				  - original goal.  Added commenting.

#ifndef _DISKVOLUME_H_
#define _DISKVOLUME_H_

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500		// Need NT 5.0 for Volume functions
#endif // _WIN32_WINNT

#include "windows.h"
#include "tchar.h"
#include "winioctl.h"
#include <vector>

using namespace std;

typedef BOOL (WINAPI *lpfnGetVolumePathNamesForVolumeNameA)(LPCSTR lpszVolumeName, 
															LPCH lpszVolumePathNames, 
															DWORD cchBufferLength, 
															PDWORD lpcchReturnLength);

typedef BOOL (WINAPI *lpfnGetVolumePathNamesForVolumeNameW)(LPCWSTR lpszVolumeName,
															LPCH lpszVolumePathNames, 
															DWORD cchBufferLength, 
															PDWORD lpcchReturnLength);

#ifdef GetVolumePathNamesForVolumeName
#undef GetVolumePathNamesForVolumeName
#endif

#ifdef UNICODE
#define lpfnGetVolumePathNamesForVolumeName lpfnGetVolumePathNamesForVolumeNameW
#define GVPNFVN "GetVolumePathNamesForVolumeNameW"
#else
#define lpfnGetVolumePathNamesForVolumeName lpfnGetVolumePathNamesForVolumeNameA
#define GVPNFVN "GetVolumePathNamesForVolumeNameA"
#endif // UNICODE


#endif // _DISKVOLUME_H_