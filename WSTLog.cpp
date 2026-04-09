#define _UNICODE
#define UNICODE

#include "stdafx.h"
#include "WSTLog.h"
#include "SysCallsLog.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include "tstl.h"
using namespace std;

/* Declarations : =================================
	WSTLog(const char *szLogFile, int iMode);

public:
	void SetLogFile(const char *szLogFile);
	const char* GetLogFile(void);
	void Write(const char *szMessage);
	int Write(const char *szMessage, int iPriority);
	void SetMode(int iMode);
================================================ */

extern string to_utf8(const wstring &);

WSTLog::WSTLog(void)
{
	Mode = WSL_NEW;		// Create a new file when ready
	IsLogging = false;	// Default is logging turned OFF
	memset(LogFile, 0, sizeof(LogFile));	// No file name
}

WSTLog::~WSTLog(void)
{
//	Close();
}

WSTLog::WSTLog(const TCHAR *szLogFile, int iMode)
{
	SetMode(iMode);
	if(SetLogFile(szLogFile) == WSL_SUCCESS)
	{
		FILE *logFile=NULL;
		IsLogging = true;	// Turn logging on
		if(_tfopen_s(&logFile, szLogFile, _T("w")) != 0)
		{
			if(errno == ENOMEM)
				exit(0);//Most likely not enough space on the drive
		}
		TCHAR cwd[MAX_PATH];
		INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
		GetCurrentDirectory(MAX_PATH,cwd);
		//cout<<cwd<<endl;
		if(logFile)
		{
			_ftprintf_s(logFile, _T("%s"), _T("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"));
			_ftprintf_s(logFile,_T("%s"),_T("<?xml-stylesheet type=\"text/xsl\" href=\"scanlog.xslt\"?>\n"));

			
			_ftprintf_s(logFile, _T("<Scanlog>\r\n"));
			//_ftprintf_s(logFile, "<Log>\r\n");
		}
		fclose(logFile);
		SetMode(WSL_APPEND | (Mode & ~WSL_NEW));
	}
	else
	{
		IsLogging = false;	// Can't log, the file name is bad
		// Caller can check Extended Error for reason
	}
}

int WSTLog::SetLogFile(const TCHAR *szLogFile)
{
	// Make sure the file name is a valid one
	if((szLogFile != NULL) && (_tcslen(szLogFile) < 254))
	{
		_tcsncpy_s(LogFile, sizeof(LogFile), szLogFile, _tcslen((szLogFile)));
		return WSL_SUCCESS;
	}
	else
	{
		_stprintf_s(ExtendedError, sizeof(ExtendedError), _T("Specified filename is invalid."));
		return WSL_ERROR;
	}
}

const TCHAR* WSTLog::GetLogFile(void)
{
	return LogFile;
}

int WSTLog::Write(const char *szMessage)
{
	// Default message type will be notes
	return Write(szMessage, WSLMSG_NOTE);
}

int WSTLog::Write(const wchar_t *szMessage, int iMsgType)
{
	return Write(to_utf8(szMessage).c_str(), iMsgType);
}

int WSTLog::Write(const char *szMessage, int iMsgType)
{
	FILE *pLogFile = NULL;
	time_t unixTime;
	struct tm theTime;
	char szTimeString[255];
	char szMsgTag[255];

	// Filter out user notes and debug messages if we're not logging verbosely
	// keeping in mind it's not an error if they are these types of messages
	if(!(Mode & WSL_VERBOSE))
	{
		if(iMsgType & (WSLMSG_NOTE | WSLMSG_DEBUG))
			return WSL_SUCCESS;
	}

	if(Mode & WSL_APPEND)	// Append to a file
		_tfopen_s(&pLogFile, LogFile, _T("a"));
	
	if(Mode & WSL_NEW)	// Create a new log file
		_tfopen_s(&pLogFile, LogFile, _T("w"));

	// Message Tags
	memset(szMsgTag, 0, 255);

	if(iMsgType & WSLMSG_DEBUG)
		strcat_s(szMsgTag, 255, "<MSG Type=\"DEBUG\"> ");
	if(iMsgType & WSLMSG_ERROR) 
		strcat_s(szMsgTag, 255, "<MSG Type=\"ERROR\"> ");
	if(iMsgType & WSLMSG_STATUS)
		strcat_s(szMsgTag, 255, "<MSG Type=\"STATUS\"> ");
	if(iMsgType & WSLMSG_NOTE)
		strcat_s(szMsgTag, 255, "<MSG Type=\"NOTE\"> ");
	if(iMsgType & WSLMSG_PROF)
		strcat_s(szMsgTag, 255, "<MSG Type=\"PROFILE\"> ");

	if(!pLogFile)
	{
		_stprintf_s(ExtendedError, sizeof(ExtendedError), _T("Could not open log file. Error %d"), errno);
		return WSL_ERROR;
	}

	// Change the mode to WSL_APPEND since the log file is there now.
	//SetMode(WSL_APPEND | (Mode & ~WSL_NEW));

	// Timestamp
	time(&unixTime);
	localtime_s(&theTime, &unixTime);
	
	szTimeString[0] = 0;
    asctime_s(szTimeString, sizeof(szTimeString), &theTime);
	if(szTimeString[strlen(szTimeString) - 1] == '\n')
		szTimeString[strlen(szTimeString) - 1] = '\0';

	if(fprintf_s(pLogFile, "%s %s : %s </MSG>", szMsgTag, szTimeString, szMessage) < 0)
	{
		_stprintf_s(ExtendedError, sizeof(ExtendedError), _T("Could not write string to log file. Error %d"), errno);
		return WSL_ERROR;
	}
	else
		fprintf_s(pLogFile, "\n");

	fflush(pLogFile);
	fclose(pLogFile);

	return WSL_SUCCESS;
}

int WSTLog::SetMode(int iMode)
{
	if((iMode > 0) && (iMode < WSL_MAXMODE))
	{
		Mode = iMode;
		return WSL_SUCCESS;
	}
	else
	{
		_stprintf_s(ExtendedError, sizeof(ExtendedError), _T("Invalid mode: %d"), iMode);
		return WSL_ERROR;
	}
}

void WSTLog::Close(void)
{
	FILE *logFile=NULL;
	static int count=1;
	tstring scanLogName;
	TCHAR cwdBuf[MAX_PATH];
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH,cwdBuf);
	scanLogName=cwdBuf;
	scanLogName=scanLogName+_T("\\Forensic\\AuditReport_incomplete.xml");

	
	_tfopen_s(&logFile,scanLogName.c_str(), _T("a")); 

	if(logFile && count == 1)
	{
		
		_ftprintf_s(logFile, _T("</Scanlog>\r\n"));
		count++;
	} 
	if (logFile != NULL)
		fclose(logFile);
	
}


//Don't forget to delete msg from the caller after use
void WSTLog::ErrorString(unsigned long errorCode,TCHAR **msg)
{

    LPVOID lpDisplayBuf;

	*msg = new TCHAR[2048];

    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        *msg,
        2048, NULL );


}


void WSTLog::FormatLogMsg(TCHAR *fmt,...)
{
		va_list args;
		TCHAR *s;
		TCHAR fmtbuf[MAX_PATH];
		int d;

		va_start(args,fmt);

		memset(LogMsg,0,LOG_MSG_BUFSIZE);
		memset(fmtbuf,0,MAX_PATH);


		for(TCHAR *p=fmt;*p!='\0';p++)
		{
			if(*p!= '%')
			{
				continue;
			}

			switch(*(++p))
			{
				case 's':
					s=va_arg(args,TCHAR *);
					_tcscat_s(LogMsg,LOG_MSG_BUFSIZE,s);
					break;
				case 'd':
					d=va_arg(args,int);
					_itot_s(d,fmtbuf,MAX_PATH,10);
					_tcscat_s(LogMsg,LOG_MSG_BUFSIZE,fmtbuf);
					break;

			}
		}

		va_end(args);
}


#undef _UNICODE
#undef UNICODE