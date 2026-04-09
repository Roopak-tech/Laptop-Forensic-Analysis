#define _UNICODE
#define UNICODE
//==============================================================================
// Event Log Parser (EventLogParser.h)
// (c) 2008 WetStone Technologies
// Abstract: Parses and dumps Windows event logs to XML format
//==============================================================================
// Some notes, the right way to do this would be to not let the file output
// be dependent within the actual code itself, and to store everything to data
// structures.  This would in turn increase the memory footprint of the app
// however, as everything would need to be in memory and then serialized out
// in a big glob.  As such, we are interlacing the code with file IO, even
// though that in itself causes some "design" headaches.
//==============================================================================
#pragma once

#ifndef _EVENTLOGPARSER_H_
#define _EVENTLOGPARSER_H_

// Target platform/SDK defines
// Commenting this out for the time being as we want to make sure we're
// 64-bit compatible too.
//#ifndef _USE_32BIT_TIME_T
//#define _USE_32BIT_TIME_T		// Constrain time_t to 32 bits
//#endif

#ifndef _WINNT_WIN32
#define _WINNT_WIN32 0x0400		// Target platform is NT4 or higher (Win95+)
#endif

//#ifndef _UNICODE
//#define _UNICODE				// Unicode support
//#endif

//#ifndef UNICODE
//#define UNICODE					// More Unicode support
//#endif	

#include <stdio.h>
#include "windows.h"	// For base types and such
#include "tchar.h"		// For TCHAR support
#include "HashedXML.h"
#include <vector>
using namespace std;

// Import the Windows Advanced API for registry and event log functions
#pragma comment(lib, "advapi32.lib")

// Defines to make life easier
#define EVENTLOG_ROOT	_T("SYSTEM\\CurrentControlSet\\Services\\Eventlog")
#define LOGFILE_KEY		_T("File")
#define MESSAGEFILE_KEY	_T("EventMessageFile")

// Combination struct for the log name and it's associated path
typedef struct _STRUCT_LogAndPath {
	TCHAR tszLogName[MAX_PATH + 1];			// Name of the event log
	TCHAR tszLogPath[MAX_PATH + 1];			// Actual path of the event log
} LogAndPath, *PLogAndPath;

// Struct for an event message source
typedef struct _STRUCT_SourceAndPath {
	TCHAR tszSourceName[MAX_PATH + 1];		// Name of event source
	TCHAR tszSourcePath[MAX_PATH + 1];		// Actual path to source message file
} SourceAndPath, *PSourceAndPath;

// I'll finish this eventually...
typedef struct _STRUCT_EventRecord {
	DWORD dwEventID;
	DWORD dwEventType;
	TCHAR tczSource[MAX_PATH + 1];
	TCHAR tczComputer[MAX_PATH + 1];
	vector<TCHAR> vtzEventText;
	vector<BYTE> vecEventData;
} EventRecord, *PEventRecord;

// The right way to do it would be to add in all of the appropriate stuff in a nice master structure,
// but this way would take a ton of memory as mentioned above.  We'll leave it in here though, just in case
// we port the UI to C++... :)
typedef struct _STRUCT_EventLog {
	TCHAR tszLogName[MAX_PATH + 1];			// Name of the event log
	TCHAR tszLogPath[MAX_PATH + 1];			// Path to the event log file
	DWORD dwNumRecords;						// Number of event records in this log
	DWORD dwNumSources;						// Number of exposed event message sources for this log
	vector<EventRecord> vecEventRecords;	// Array of event records for this log
	vector<SourceAndPath> vecSources;		// Array of associated sources and message file paths
} EventLog, *PEventLog;

class EventLogParser
{
public:
	EventLogParser(void);
	~EventLogParser(void);

	int Start(HashedXMLFile &repository);		// Entry point for the class

private:
	FILE *pFileHandle;						// Persistent handle for XML output
	int iNumberOfLogs;						// Number of event logs listed in the registry
	vector<LogAndPath> vecEventLogs;		// The actual array of logs

	int FindEventLogs(void);				// Searches the registry for event logs
	int GetLogSources(HashedXMLFile &repository, LPCTSTR ptszLog);		// Parse out all the event message sources for a given log "ptszLog"
	int ParseLog(HashedXMLFile &repository, LPCTSTR ptszLog,string os);			// Parses the log file specified by "ptszLog"
	int GetEntryCount(LPCTSTR ptszLog);		// Gets the number of records in "ptszLog"

	int StartLog(LPCTSTR ptszFileName);		// Open the XML file "ptszFileName" for output (XML)
	int CloseLog(void);						// Closes the XML file
};

#endif // _EVENTLOGPARSER_H_
