#ifndef EMAILEXTRACT
#define EMAILEXTRACT

#include <Windows.h>
#include <iostream>
#include <string.h>
#include <sstream>
#include <stdlib.h>
#include "FileHasher.h"
#include "WSTLog.h"
using namespace std;

#define MINIMUM_SPACE_REQUIRED 5*1024*1024 //5MB (minimum amount of space required for a triage)

#define MAILFILECAPTURE_EXPORT __declspec(dllexport)

#define MFL 20
#define MF TEXT("MailFile")

// Default registry paths, should be generally the same
#define REGPATHOUTLOOK  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows Messaging Subsystem\\Profiles\\Outlook\\")
#define REGPATHLIVEMAIL TEXT("Software\\Microsoft\\Windows Live Mail\\")
#define PROFILESINITHUNDERBIRD TEXT("%USERPROFILE%\\AppData\\Roaming\\Thunderbird\\profiles.ini")

#ifdef DEBUG
#define dcout(x)  cout<<x<<endl
#define dcout_m(x,m) cout<<x<<m<<endl
#define dwcout(x)  wcout<<x<<endl
#define dwcout_m(x,m) wcout<<x<<m<<endl
#endif

#define BASE 10

//Success "errors"
#define SUCCESS 100

//Reg Errors -1xx
#define ERROR_REGOPENREGQUERY -100
#define ERR_REG_OPEN_KEY -101
#define ERR_REG_QUERY_INFO -102
#define ERR_REG_ENUM_KEY_SERIAL -103
#define ERR_REG -104

//Function Failures -2xx
#define ERR_FIND_FIRST -200
#define ERR_ENVAR -201

//Copy Failures -3xx
#define ERR_COPY_FAIL -300
#define CPY_INSUFF_SPACE_ERR -301
#define ERR_FIND_FIRST -302
#define ERR_SHW_CPY -303

//General Failures -4xx
#define ERR_FILE_NOT_EXIST -400

#endif


MAILFILECAPTURE_EXPORT int MailFileCapture(WSTLog &AppLog);