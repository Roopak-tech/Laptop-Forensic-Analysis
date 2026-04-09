// Portable logger for all Wetstone Apps
#pragma once

#ifndef _WSTLOG_H_
#define _WSTLOG_H_

// WetStone Logging Logger Options
#define WSL_NEW		1	// Create a new log file every time, overwrites if exists
#define WSL_APPEND	2	// Appends to a file, creates new if it doesn't exist
#define WSL_VERBOSE 4	// Verbose logging, reserved
#define WSL_MAXMODE 7	// Maximum value for mode setting

// WetStone Logging Error Codes
#define WSL_ERROR	0	// 
#define WSL_SUCCESS 1	//

// WetStone Logging Message types
#define WSLMSG_NOTE		1	// User-type message
#define WSLMSG_STATUS	2	// Application status message
#define WSLMSG_ERROR	4	// Application error message
#define WSLMSG_DEBUG	8	// Application debugging message
#define WSLMSG_PROF     16   // Profiling message

#define LOG_MSG_BUFSIZE 2*260+2048



#ifdef __cplusplus
extern "C" {
#endif

class WSTLog
{
public:
	WSTLog(void);
	~WSTLog(void);

	WSTLog(const TCHAR *szLogFile, int iMode);

public:
	TCHAR LogMsg[LOG_MSG_BUFSIZE];

	int SetLogFile(const TCHAR *szLogFile);
	const TCHAR* GetLogFile(void);
	int Write(const char *szMessage);
	int Write(const wchar_t *szMessage, int iMsgType);
	int Write(const char *szMessage, int iMsgType);
	int SetMode(int iMode);
    void Close(void);
	void ErrorString(unsigned long errorCode,TCHAR **msg);
	void FormatLogMsg(TCHAR *fmt,...);

private:
	bool IsLogging;
	TCHAR LogFile[255];
	int	 Mode;
	TCHAR ExtendedError[255];
	
};

#ifdef _DEBUG
   extern WSTLog AppLog;//("scanlog_incomplete.xml", WSL_NEW | WSL_VERBOSE);
#else
   extern WSTLog AppLog;//("scanlog_incomplete.xml", WSL_NEW);
#endif // _DEBUG

#define LOG_SYSTEM_ERROR(uslatt_msg)\
	TCHAR *msg;\
	memset(AppLog.LogMsg,0,LOG_MSG_BUFSIZE);\
	AppLog.ErrorString(GetLastError(),&msg);\
	AppLog.FormatLogMsg(_T("%s %s %s"),uslatt_msg,_T(", System Error: "),msg);\
	AppLog.Write(AppLog.LogMsg, WSLMSG_ERROR);\
	delete [] msg;

#ifdef __cplusplus
};
#endif

#endif // _WSTLOG_H_