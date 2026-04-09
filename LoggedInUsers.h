#define _UNICODE
#define UNICODE

#pragma once

#ifndef _WINNT_WIN32
#define _WINNT_WIN32 0x0400		// Target platform is NT4 or higher (Win95+)
#endif


#include "windows.h"
#include "tchar.h"
#include "lm.h"
#include <vector>

using namespace std;

#pragma comment(lib, "netapi32.lib")

class LoggedInUsers
{
public:
	LoggedInUsers(void);
	~LoggedInUsers(void);

	int GetLoggedInUsers(void);

	vector<LPWSTR> vecUsers;
};
