#pragma once

#include <tchar.h>
#include "WSTRemote.h"

#pragma warning(disable:4996)

#ifdef _UNICODE
#define TSTRING std::wstring
#else
#define TSTRING std::string
#endif

// The main search functionality
unsigned int FindFilesWST(const TCHAR *FilePath, const TCHAR *FileMask, const int Recurse, WSTRemote *pRemote);

// Plugins to use on found files
bool CopyFile_Plugin(const TCHAR *FileSource, const TCHAR *RootDestination);