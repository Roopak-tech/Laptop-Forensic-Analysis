#define _UNICODE
#define UNICODE

#include "stdafx.h"
#include "EventLogParser.h"
#include "time.h"
#include "HashedXML.h"
#include <map>
#include "WSTRemote.h"
#include "WSTLog.h"
#include "SysCallsLog.h"
extern string to_utf8(const wstring&);
extern string os_name;

extern TCHAR g_tempBuf[MAX_PATH];
//int FindEventLogs(void);					// Searches the registry for event logs
//int GetLogSources(LPCTSTR ptszLog);		// Parse out all the event message sources for a given log "ptszLog"
//int ParseLog(LPCTSTR ptszLog);			// Parses the log file specified by "ptszLog"
//int GetEntryCount(LPCTSTR ptszLog);		// Gets the number of records in "ptszLog"
//int StartLog(LPCTSTR ptszFileName);		// Open the XML file "ptszFileName" for output (XML)
//int CloseLog(void);						// Closes the XML file

EventLogParser::EventLogParser(void)
{
	pFileHandle = NULL;
	iNumberOfLogs = 0;
}

EventLogParser::~EventLogParser(void)
{
}

int EventLogParser::Start(HashedXMLFile &repository)
{
	time_t tt;
    TCHAR szValue[35];


    repository.OpenNode("EventLog");

	// Parse out logs here
	iNumberOfLogs = FindEventLogs();

	/*for(int i=0;i<iNumberOfLogs;i++)
	{
		cout<<vecEventLogs[i].tszLogName<<endl;
	}
	*/

	if(iNumberOfLogs)
	{
		// Get log infos
		for(int i = 0; i < iNumberOfLogs; i++)
		{
			if(_tcsncmp(vecEventLogs[i].tszLogName,_T("Security"),8))
			{
				continue;
			}//Uncomment this to capture all logs
            repository.OpenNodeWithAttributes("Log");
            repository.WriteAttribute("name", vecEventLogs[i].tszLogName);
            repository.Write(">");
            repository.OpenNode("LogInfo");
			time(&tt);
            _tctime_s(szValue, sizeof(szValue)/sizeof(TCHAR), &tt);
            repository.WriteNodeWithValue("LocalTimeStamp", szValue);
            repository.WriteNodeWithValue("File", vecEventLogs[i].tszLogPath);
            _stprintf_s(szValue, sizeof(szValue)/sizeof(TCHAR), _T("%d"), GetEntryCount(vecEventLogs[i].tszLogName));
            repository.WriteNodeWithValue("Entries", szValue);
			// This line won't work right given the "inlining" of all the file IO.
			// _ftprintf(pFileHandle, _T("      <SourceCount>%d</SourceCount>\n"), GetLogSources(vecEventLogs[i].tszLogName));
			GetLogSources(repository, vecEventLogs[i].tszLogName);	// This will simply do nothing if no sources are found.
            repository.CloseNode("LogInfo");
			ParseLog(repository, vecEventLogs[i].tszLogName,os_name);
            repository.CloseNode("Log");
		}
	}
	else {
		_ftprintf(pFileHandle, _T("<ErrorMessage>No Logs Found</ErrorMessage>\n"));
		 repository.CloseNode("EventLog"); //-RJM
		 return 0;//Error generating event log information.  --RJM
	}

    repository.CloseNode("EventLog");

	return 1;
}

int EventLogParser::FindEventLogs(void)
{
	// Switch to DWORDs and such for Win32 API calls
	DWORD dwRetVal = 0;
	DWORD dwTempVal = 0;
	DWORD dwNumLogs = 0;
	DWORD dwMaxKeyLen = 0;
	DWORD dwMaxDataLen = 0;
	DWORD dwValueType = 0;
	DWORD dwIdx = 0;
	HKEY hkLogRoot = 0;
	HKEY hkThisLog = 0;
	TCHAR tszTempPath[MAX_PATH + 1];

	LogAndPath tempLAP;

	AppLog.Write("In FindEventLogs",WSLMSG_STATUS);
	INC_SYS_CALL_COUNT(RegOpenKeyEx); // Needed to be INC TKH
	dwRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, EVENTLOG_ROOT, 0, KEY_READ, &hkLogRoot);

	if(dwRetVal != ERROR_SUCCESS)
	{
		AppLog.Write("No event logs found (RegOpenKeyEx failed)",WSLMSG_ERROR);
		return 0;
	}

	// Iterate through all supposed event logs
	do
	{
		memset(&tempLAP, 0, sizeof(LogAndPath));

		// Store the "name" of the log directly to the LAP object
		dwMaxKeyLen = MAX_PATH;
		INC_SYS_CALL_COUNT(RegEnumKeyEx); // Needed to be INC TKH
		dwRetVal = RegEnumKeyEx(hkLogRoot, dwIdx++, tempLAP.tszLogName, &dwMaxKeyLen, NULL, NULL, NULL, NULL);

		if(dwRetVal == ERROR_SUCCESS)
		{
			INC_SYS_CALL_COUNT(RegOpenKeyEx); // Needed to be INC TKH
			dwTempVal = RegOpenKeyEx(hkLogRoot, tempLAP.tszLogName, 0, KEY_READ, &hkThisLog);
			
			// Validate the key opened and retrieve the path
			if(dwTempVal == ERROR_SUCCESS)
			{
				dwMaxDataLen = MAX_PATH;
				dwValueType = 0;

				INC_SYS_CALL_COUNT(RegQueryValueEx); // Needed to be INC TKH
				dwTempVal = RegQueryValueEx(hkThisLog, LOGFILE_KEY, NULL, &dwValueType, (LPBYTE)tszTempPath, &dwMaxDataLen);

				if(dwTempVal == ERROR_SUCCESS)
				{
					// We've got a live one here!
					if(dwValueType == REG_EXPAND_SZ)
					{
						// I'm not going to test this on the principle that it "shouldn't" fail
						// but in all likelyhood, it's going to because I said that.
						ExpandEnvironmentStrings(tszTempPath, tempLAP.tszLogPath, MAX_PATH);
					}
					else
					{
						_tcscpy_s(tempLAP.tszLogPath, MAX_PATH, tszTempPath);
					}
				}

				// If the key doesn't have any valid values, we're going to leave the path blank
				// to denote no log file is available.
				INC_SYS_CALL_COUNT(RegCloseKey); // Needed to be INC TKH
				RegCloseKey(hkThisLog);
			}
			// else{} It's not a valid event log according to the standard format so we ignore it.
			// Insert it into the array
			vecEventLogs.push_back(tempLAP);

			// Add a valid (or named) event log to the log count
			dwNumLogs++;
		} // end if
	} while(dwRetVal != ERROR_NO_MORE_ITEMS);	// Enum will return ERROR_NO_MORE_ITEMS once there... no more items!

	INC_SYS_CALL_COUNT(RegCloseKey); // Needed to be INC TKH
	RegCloseKey(hkLogRoot);

	return dwNumLogs;
}

int EventLogParser::GetLogSources(HashedXMLFile &repository, LPCTSTR ptszLog)
{
	HKEY hkLogRoot = 0;
	HKEY hkSource = 0;
	
	DWORD dwNumSources = 0;
	DWORD dwMaxSourceStrLen = MAX_PATH;
	DWORD dwRetVal = 0;
	DWORD dwTempVal = 0;
	DWORD dwTempIdx = 0;
	DWORD dwDataType = 0;
	
	SourceAndPath tempSAP;

	AppLog.Write("In GetLogSources",WSLMSG_STATUS);

	TCHAR tszSourceKey[MAX_PATH + 1] = _T("");

	// Ensure ptszLog is not NULL or blank, since opening either as a key will actually work.
	if(!ptszLog || !_tcscmp(ptszLog, _T("")))
	{
		AppLog.Write("Key was null (returning)",WSLMSG_ERROR);
		return 0;
	}

	memset(tszSourceKey, 0, MAX_PATH + 1);
	_stprintf_s(tszSourceKey, MAX_PATH, _T("%s\\%s"), EVENTLOG_ROOT, ptszLog);

	INC_SYS_CALL_COUNT(RegOpenKeyEx); // Needed to be INC TKH
	dwRetVal = RegOpenKeyEx(HKEY_LOCAL_MACHINE, tszSourceKey, 0, KEY_READ, &hkLogRoot);

	if(dwRetVal != ERROR_SUCCESS)
	{
		memset(g_tempBuf,0,MAX_PATH);
		_stprintf_s(g_tempBuf,MAX_PATH,_T("RegOpenKeyEx failed in GetLogSources %d"),GetLastError());
		AppLog.Write(g_tempBuf,WSLMSG_STATUS);//Chnages RJM (03/28/2011)
		return 0;
	}

	// Should have the key for the log open now, we can hopefully iterate through all the registered sources
	
	do
	{
		// Reset and enumerate the next key, dwTempIdx is our enumeration index
		dwMaxSourceStrLen = MAX_PATH;
		memset(&tempSAP, 0, sizeof(SourceAndPath));
		INC_SYS_CALL_COUNT(RegEnumKeyEx); // Needed to be INC TKH
		dwRetVal = RegEnumKeyEx(hkLogRoot, dwTempIdx++, tempSAP.tszSourceName, &dwMaxSourceStrLen, NULL, NULL, NULL, NULL);
		
		// If we successfully enumerated a key...
		if(dwRetVal == ERROR_SUCCESS)
		{
            repository.OpenNodeWithAttributes("Source");
            repository.WriteAttribute("name", tempSAP.tszSourceName);
            repository.Write(">");
			// Open up the source and retrieve the source message path
			INC_SYS_CALL_COUNT(RegOpenKeyEx); // Needed to be INC TKH
			dwTempVal = RegOpenKeyEx(hkLogRoot, tempSAP.tszSourceName, 0, KEY_READ, &hkSource);

			// Opened the source key...
			if(dwTempVal == ERROR_SUCCESS)
			{
				// Reuse a few variables
				dwMaxSourceStrLen = MAX_PATH;
				dwDataType = 0;

				// Query the message file value
				INC_SYS_CALL_COUNT(RegQueryValueEx); // Needed to be INC TKH
				if(RegQueryValueEx(hkSource, MESSAGEFILE_KEY, NULL, &dwDataType, (LPBYTE)tszSourceKey, &dwMaxSourceStrLen) == ERROR_SUCCESS)
				{
					// System expanded variable type
					if(dwDataType == REG_EXPAND_SZ)	// I have a feeling this should be OR'd with REG_SZ too.
					{
						// I'm not going to test this one as it shouldn't be more than MAX_PATH anyways
						// but in all honesty this would be better served using a dynamic buffer, but
						// beggars can't be choosers now can they.
						ExpandEnvironmentStrings(tszSourceKey, tempSAP.tszSourcePath, MAX_PATH);
					}
					else	// Else it's just a string?  (I hope it's not not a string!)
					{
						_tcscpy_s(tempSAP.tszSourcePath, MAX_PATH, tszSourceKey);
					}

                    repository.WriteNodeWithValue("MessageFile", tempSAP.tszSourcePath);
				}
				
				// Valid source, event if it doesn't have a path
				dwNumSources++;
			}	// end if

            repository.CloseNode("Source");

			// We're done now, thanks.
			INC_SYS_CALL_COUNT(RegCloseKey); // Needed to be INC TKH
			RegCloseKey(hkSource);
		} // end if (enum)
		else
		{
			memset(g_tempBuf,0,MAX_PATH);
			_stprintf_s(g_tempBuf,MAX_PATH,_T("RegEnumKeyEx failed in GetLogSources %d"),GetLastError());
			AppLog.Write(g_tempBuf,WSLMSG_STATUS);
		}
			
	} while(dwRetVal != ERROR_NO_MORE_ITEMS);	// Keep enumerating until there are no more items. Brilliant!

	INC_SYS_CALL_COUNT(RegCloseKey); // Needed to be INC TKH
	RegCloseKey(hkLogRoot);

	return dwNumSources;
}

#ifdef _UNICODE

tstring XMLDelimitString(tstring szSource)
{
    tstring::size_type pos;
    tstring searchString, replaceString;
    tstring newString;
    pos = 0;
    searchString = _T(">");
    replaceString = _T("&qt;");
    while ( (pos = szSource.find(searchString, pos)) != tstring::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = _T("<");
    replaceString = _T("&lt;");
    while ( (pos = szSource.find(searchString, pos)) != tstring::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = _T("&");
    replaceString = _T("&amp;");
    while ( (pos = szSource.find(searchString, pos)) != tstring::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = _T("\"");
    replaceString = _T("&quot;");
    while ( (pos = szSource.find(searchString, pos)) != tstring::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = _T("\'");
    replaceString = _T("&apos;");
    while ( (pos = szSource.find(searchString, pos)) != tstring::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = _T("™");
    replaceString = _T("(tm)");
    while ( (pos = szSource.find(searchString, pos)) != tstring::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = _T("®");
    replaceString = _T("(R)");
    while ( (pos = szSource.find(searchString, pos)) != tstring::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    return szSource;
}

#endif
//
//  This function delimits xml strings
//
//
string XMLDelimitString(string szSource)
{
    string::size_type pos;
    string searchString, replaceString;
    string newString;
    pos = 0;
    searchString = ">";
    replaceString = "&qt;";
    while ( (pos = szSource.find(searchString, pos)) != string::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = "<";
    replaceString = "&lt;";
    while ( (pos = szSource.find(searchString, pos)) != string::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = "&";
    replaceString = "&amp;";
    while ( (pos = szSource.find(searchString, pos)) != string::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = "\"";
    replaceString = "&quot;";
    while ( (pos = szSource.find(searchString, pos)) != string::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = "\'";
    replaceString = "&apos;";
    while ( (pos = szSource.find(searchString, pos)) != string::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = "™";
    replaceString = "(tm)";
    while ( (pos = szSource.find(searchString, pos)) != string::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }
    pos = 0;
    searchString = "®";
    replaceString = "(R)";
    while ( (pos = szSource.find(searchString, pos)) != string::npos) 
    {
        szSource.replace(pos, searchString.size(), replaceString);
        ++pos;
    }

    
    return szSource;
}

//
//  This function delimits xml strings
//
//
string XMLDelimitString(const char *szValue)
{
    string szStringValue;

    szStringValue = szValue;
    return XMLDelimitString(szStringValue);
}


#ifdef _UNICODE
//
//  This function delimits xml strings
//
//
tstring XMLDelimitString(const wchar_t *szValue)
{
    wstring szStringValue;

    szStringValue = szValue;
    return XMLDelimitString(szStringValue);
}

#endif


/*
int EventLogParser::ParseLog(HashedXMLFile &repository, LPCTSTR ptszLog)
{
	HANDLE hEventLog = NULL;
	HMODULE hMsgDLL = NULL;
	HKEY hkMsgSource = NULL;
	DWORD dwNumRecords = 0;
	DWORD dwDataLength = 0;
	DWORD dwTextLength = 0;
	DWORD dwBufferLength = 0;
	DWORD dwOldestRecord = 0;
	DWORD dwBytesRead = 0;
	DWORD dwBytesNeeded = 0;
	DWORD dwBufferSize = 0;
	DWORD dwRetVal = 0;
	DWORD dwMsgFileLen = 0;
	DWORD dwRegKeyType = 0;
	DWORD dwTempIdx = 0;
	DWORD dwUserNameLen = 0;
	DWORD dwDomainNameLen = 0;
	BYTE fakeBuffer = 0;
	EVENTLOGRECORD *pELR = NULL;
	LPWSTR pwzSource = NULL;
	LPWSTR pwzComputer = NULL;
	LPWSTR pwzStrings = NULL;
	char *pszStrings = NULL;
	DWORD_PTR *pDWPStrings = NULL;
	TCHAR tszEventSource[MAX_PATH + 1] = _T("");
	TCHAR tszMessageFile[MAX_PATH + 1] = _T("");
	TCHAR tszUserName[MAX_PATH + 1] = _T("");
	TCHAR tszDomainName[MAX_PATH + 1] = _T("");
    TCHAR szValue[50];
	LPTSTR ptzMessageBuffer = NULL;
	LPTSTR ptzToken = NULL;
	LPTSTR ptzNextToken = NULL;
	SID_NAME_USE SNU;
    string szMessageBuffer;
    DWORD dwRecordsToGet;

	// Ensure we have an actual log to open first
	if(!ptszLog || !_tcscmp(ptszLog, _T("")))
	{
		return 0;
	}

	// Get the number of event log entries for this log.
	dwNumRecords = GetEntryCount(ptszLog);
	
	// Open the event log up, this is where the fun really begins.
	hEventLog = OpenEventLog(NULL, ptszLog);
	
	if(hEventLog == NULL)
    {
        DWORD dwError = GetLastError();

        if (dwError == ERROR_PRIVILEGE_NOT_HELD)
        {
        }
		return 0;
    }

	// Find the "offset" of the oldest entry...
	if(!GetOldestEventLogRecord(hEventLog, &dwOldestRecord))
		return 0;

    dwRecordsToGet = dwNumRecords; 
    if (dwRecordsToGet > 5000)
    {
        dwRecordsToGet = 5000;
    }
	// Iterate through the log and dump everything.
	for(DWORD i = dwOldestRecord; i < dwOldestRecord + dwRecordsToGet; i++)
	{
		dwRetVal = 0;
		dwBytesRead = 0;
		dwBytesNeeded = 0;

		if(!ReadEventLog(hEventLog, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ, i, (LPVOID)&fakeBuffer, 1, &dwBytesRead, &dwBytesNeeded))
		{
			// We should get ERROR_INSUFFICIENT_BUFFER as we only gave it 1 byte for the whole record,
			// but the good news is that we now know how much buffer we really do need.
			if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				dwBufferSize = dwBytesNeeded;
				pELR = (EVENTLOGRECORD*)malloc(dwBufferSize);
				memset((LPBYTE)pELR, 0, dwBufferSize);	// Hopefully this won't blow the heap

				dwBytesNeeded = 0;
				dwBytesRead = 0;

				// Read the event log record in, technically it shouldn't fail, but if it does, well...
				// I guess we'll ignore this record and keep on truckin'.
				if(ReadEventLog(hEventLog, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ, i, (LPVOID)pELR, dwBufferSize, &dwBytesRead, &dwBytesNeeded))
				{	
                    char *pValue;
					// This one is easy, the source record is right after the EVENTLOGRECORD struct
					pwzSource = (LPWSTR)((LPBYTE)pELR + sizeof(EVENTLOGRECORD));
					
                    pValue = (char *)pwzSource;
                    pValue = pValue + ((strlen((const char *)pwzSource) + 1) * sizeof(char));
					// This one is a little more difficult, as it sits after the wide null at the end of pwzSource
					pwzComputer = (LPWSTR)pValue;

                    // create an entry for this event.
                    repository.OpenNodeWithAttributes("Event");
                    sprintf_s(szValue, sizeof(szValue), "%d", pELR->RecordNumber);                    
                    repository.WriteAttribute("record", szValue);
                    repository.Write(">");

                    repository.WriteNodeWithValue("Log", ptszLog);
                    sprintf_s(szValue, sizeof(szValue), "%d", pELR->EventID);                    
                    repository.WriteNodeWithValue("RawEventID", szValue);
                    sprintf_s(szValue, sizeof(szValue), "%d", LOWORD(pELR->EventID));                    
                    repository.WriteNodeWithValue("EventID", szValue);
                    switch (pELR->EventType)
                    {
                    case EVENTLOG_ERROR_TYPE:
                        repository.WriteNodeWithValue("EventType", "Error");
                        break;
                    case EVENTLOG_AUDIT_FAILURE:
                        repository.WriteNodeWithValue("EventType", "Failure");
                        break;
                    case EVENTLOG_AUDIT_SUCCESS:
                        repository.WriteNodeWithValue("EventType", "Success");
                        break;
                    case EVENTLOG_INFORMATION_TYPE:
                        repository.WriteNodeWithValue("EventType", "Information");
                        break;
                    case EVENTLOG_WARNING_TYPE:
                        repository.WriteNodeWithValue("EventType", "Warning");
                        break;
                    default:
                        repository.WriteNodeWithValue("EventType", "Unknown");
                        break;
                    }
                    // convert the event time to local, machine time
                    time_t tempTime;
                    tempTime = pELR->TimeGenerated;
                    ctime_s(szValue, sizeof(szValue),&tempTime);
                    repository.WriteNodeWithValue("EventTime", szValue);
                    repository.WriteNodeWithValue("EventComputer", (const char *)pwzComputer);
                    repository.WriteNodeWithValue("EventSource", (const char *)pwzSource);

			        // Get the actual username of the account creating this event if any
			        if(pELR->UserSidLength)
			        {
				        dwUserNameLen = MAX_PATH;
				        dwDomainNameLen = MAX_PATH;
						INC_SYS_CALL_COUNT(LookupAccountSid); // Needed to be INC TKH
				        if(LookupAccountSid(NULL, (PSID)((LPBYTE)pELR + pELR->UserSidOffset), tszUserName, &dwUserNameLen, tszDomainName, &dwDomainNameLen, &SNU))
				        {
                            repository.OpenNode("UserSID");
                            repository.Write(tszDomainName);
                            repository.Write("\\");
                            repository.Write(tszUserName);
                            repository.CloseNode("UserSID");
				        }
			        }

					// Check to see if we have to worry about inserts
					if(pELR->NumStrings > 0)
					{
     					szMessageBuffer = "";

						// Make our life easier and get the string offsets
                        // NOTE: this is being changed from wchar to char since the
                        // strings seem to be in char format.  This will need to be tested on some other
                        // system to determine if they are always chars.
						pszStrings = ((char *)pELR + pELR->StringOffset);

						// Set up an array of DWORD_PTR's for FormatMessage
						for(DWORD dwpIdx = 0; dwpIdx < pELR->NumStrings; dwpIdx++)
						{
                            szMessageBuffer = szMessageBuffer + pszStrings;
                            szMessageBuffer = szMessageBuffer + "\n";
                            if (dwpIdx < (pELR->NumStrings-1))
                            {
							   pszStrings += strlen(pszStrings) + 1;	// walk to the next insert string
                            }
						}					

			            // Need to escape the xml in here as it may contain invalid characters...
                        repository.OpenNode("EventText");
                        if (pELR->EventID == 11707)
                        {
            			szMessageBuffer = XMLDelimitString(szMessageBuffer);

                        }
                        else
                        {
            			  szMessageBuffer = XMLDelimitString(szMessageBuffer);
                        }
                        repository.Write(szMessageBuffer.c_str());
                        repository.CloseNode("EventText");
					} // end if (Insert strings)
					else
					{
						pDWPStrings = NULL;
					}



			        // Dump any "user data" associated with this one
			        if(pELR->DataLength > 0)
			        {
                        repository.OpenNode("EventData");
				        for(dwTempIdx = 0; dwTempIdx < pELR->DataLength; dwTempIdx++)
				        {
                            sprintf_s(szValue, sizeof(szValue), "%02x ", *((LPBYTE)pELR + pELR->DataOffset + dwTempIdx));
                            repository.Write(szValue);
				        }
                        repository.CloseNode("EventData");
			        }
                    repository.CloseNode("Event");

				} // end if (ReadEventLog)
			} // end if (ERROR_INSUFFICIENT_BUFFER)
			free(pELR);
			pELR = NULL;
		}	// End ReadEventLog
	} // end for

	// Yay, we're done!
	CloseEventLog(hEventLog);

	return dwNumRecords;
}
*/

/* Modified to capture only the security log events 
revert to the commented code above to capture information in the way it was done earlier prior to IDEAL work
--RJM */

//Create a Map of all the events we are interested in capturing along with category name 

TCHAR *SecEventCat[]={_T("StartUp-ShutdownActions"),_T("Login-LogoutActions"),_T("AccountActions"),_T("FirewallActions"),_T("MiscActions")};

//Map of interesting events
map<int,tstring> EventMapXP;

//populate the event map
int event_id_list[]={512,513,517,520,528,529,530,531,532,533,534,535,536,537,538,539,540,551,552,601,624,625,626,627,628,629,630,642,644,645,646,647,685,849,850,852,861};
//These refer to the SecEventCat above
int event_id_cat[]={0,0,4,4,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,4,2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,3,3};

int populateEventMapXP()
{
	AppLog.Write("Populating security event Map for XP",WSLMSG_STATUS);
	
	if((sizeof(event_id_list)/sizeof(int)) != (sizeof(event_id_cat)/sizeof(int)))
		return -1;
	for(int i=0;i<sizeof(event_id_list)/sizeof(int);i++)
		EventMapXP[event_id_list[i]]=(SecEventCat[event_id_cat[i]]);
	return 0;
}

bool EventToBeRecordedXP(int event_id)
{
	if(EventMapXP[event_id]==_T(""))
		return false;
	return true;
}

//Map of interesting events
map<int,tstring> EventMapVISTA;
 
//populate the event map
int event_id_list_vista[]={4608,4609,4616,4624,4625,4634,4646,4647,4648,4649,4697,4717,4718,4719,4720,4722,4723,4724,4725,4726,4738,4739,4740,4741,4742,4743,4781,4789,4793,4800,4801,4944,4945,4946,4947,4948,4949,4950,5148,5149,5150,5157};
//These refer to the SecEventCat above
int event_id_cat_vista[]={0,0,4,1,1,1,1,1,1,1,4,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,4,1,1,3,3,3,3,3,3,3,4,4,4,3};


bool EventToBeRecordedVISTA(int event_id)
{
	if(EventMapVISTA[event_id]==_T(""))
		return false;
	return true;
}

int populateEventMapVISTA()
{
	if(os_name=="VISTA")
	{
		AppLog.Write("Populating security event Map for Vista",WSLMSG_STATUS);
	}
	else
	{
			AppLog.Write("Populating security event Map for WIN7",WSLMSG_STATUS);
	}
	if((sizeof(event_id_list_vista)/sizeof(int)) != (sizeof(event_id_cat_vista)/sizeof(int)))
		return -1;
	for(int i=0;i<sizeof(event_id_list_vista)/sizeof(int);i++)
		EventMapVISTA[event_id_list_vista[i]]=(SecEventCat[event_id_cat_vista[i]]);
	return 0;
}

BOOL SetPrivilege( 
	HANDLE hToken,  // token handle 
	LPCTSTR Privilege,  // Privilege to enable/disable 
	BOOL bEnablePrivilege  // TRUE to enable. FALSE to disable 
);

int EventLogParser::ParseLog(HashedXMLFile &repository, LPCTSTR ptszLog,string os)
{
	HANDLE hEventLog = NULL;
	HMODULE hMsgDLL = NULL;
	HKEY hkMsgSource = NULL;
	DWORD dwNumRecords = 0;
	DWORD dwDataLength = 0;
	DWORD dwTextLength = 0;
	DWORD dwBufferLength = 0;
	DWORD dwOldestRecord = 0;
	DWORD dwBytesRead = 0;
	DWORD dwBytesNeeded = 0;
	DWORD dwBufferSize = 0;
	DWORD dwRetVal = 0;
	DWORD dwMsgFileLen = 0;
	DWORD dwRegKeyType = 0;
	DWORD dwTempIdx = 0;
	DWORD dwUserNameLen = 0;
	DWORD dwDomainNameLen = 0;
	BYTE fakeBuffer = 0;
	EVENTLOGRECORD *pELR = NULL;
	LPWSTR pwzSource = NULL;
	LPWSTR pwzComputer = NULL;
	LPWSTR pwzStrings = NULL;
	TCHAR *pszStrings = NULL;
	DWORD_PTR *pDWPStrings = NULL;
	TCHAR tszEventSource[MAX_PATH + 1] = _T("");
	TCHAR tszMessageFile[MAX_PATH + 1] = _T("");
	TCHAR tszUserName[MAX_PATH + 1] = _T("");
	TCHAR tszDomainName[MAX_PATH + 1] = _T("");
    TCHAR szValue[50];
	LPTSTR ptzMessageBuffer = NULL;
	LPTSTR ptzToken = NULL;
	LPTSTR ptzNextToken = NULL;
	SID_NAME_USE SNU;
    tstring szMessageBuffer;
    DWORD dwRecordsToGet;

	
	AppLog.Write("In Security event parse log",WSLMSG_STATUS);

	if(os=="XP")
	{
		AppLog.Write("XP operating system detected",WSLMSG_NOTE);
		populateEventMapXP();

		HANDLE hToken;
		OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken);
		SetPrivilege(hToken, SE_SECURITY_NAME, true);
		CloseHandle(hToken);
		//cout<<sizeof(event_id_list_vista)/sizeof(int)<<" "<<(sizeof(event_id_cat_vista)/sizeof(int))<<endl;

	}
	else if(os=="VISTA" || os=="WIN7")
	{
		if(os=="VISTA")
		{
			AppLog.Write("Windows Vista operating system detected",WSLMSG_PROF);
		}
		else
		{
			AppLog.Write("Windows 7 operating system detected",WSLMSG_PROF);
		}

		populateEventMapVISTA();
	}

	// Ensure we have an actual log to open first
	if(!ptszLog || !_tcscmp(ptszLog, _T("")))
	{
		return 0;
	}

	// Get the number of event log entries for this log.
	dwNumRecords = GetEntryCount(ptszLog);
	
	// Open the event log up, this is where the fun really begins.
	hEventLog = OpenEventLog(NULL, ptszLog);
	
	if(hEventLog == NULL)
    {
        DWORD dwError = GetLastError();
		memset(g_tempBuf,0,MAX_PATH);
		_stprintf_s(g_tempBuf,MAX_PATH,_T("OpenEventLog failed %d"),dwError);

		AppLog.Write(g_tempBuf,WSLMSG_ERROR);


        if (dwError == ERROR_PRIVILEGE_NOT_HELD)
        {
			AppLog.Write("Privilege not held collecting security event log",WSLMSG_ERROR);
        }
		return 0;
    }

	// Find the "offset" of the oldest entry...
	if(!GetOldestEventLogRecord(hEventLog, &dwOldestRecord))
	{
		AppLog.Write("Returning after GetOldestEventLogRecord in security event log",WSLMSG_ERROR);
		return 0;
	}

    dwRecordsToGet = dwNumRecords; 
    if (dwRecordsToGet > 5000)
    {
        dwRecordsToGet = 5000;
    }
	// Iterate through the log and dump everything.
	for(DWORD i = dwOldestRecord; i < dwOldestRecord + dwRecordsToGet; i++)
	{
		dwRetVal = 0;
		dwBytesRead = 0;
		dwBytesNeeded = 0;
		tstring cat;

	

		if(!ReadEventLog(hEventLog, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ, i, (LPVOID)&fakeBuffer, 1, &dwBytesRead, &dwBytesNeeded))
		{
			// We should get ERROR_INSUFFICIENT_BUFFER as we only gave it 1 byte for the whole record,
			// but the good news is that we now know how much buffer we really do need.
			if(GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				dwBufferSize = dwBytesNeeded;
				pELR = (EVENTLOGRECORD*)malloc(dwBufferSize);
				memset((LPBYTE)pELR, 0, dwBufferSize);	// Hopefully this won't blow the heap

				dwBytesNeeded = 0;
				dwBytesRead = 0;

				// Read the event log record in, technically it shouldn't fail, but if it does, well...
				// I guess we'll ignore this record and keep on truckin'.
				if(ReadEventLog(hEventLog, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ, i, (LPVOID)pELR, dwBufferSize, &dwBytesRead, &dwBytesNeeded))
				{	
					if(os=="XP")
					{
						
						if(!EventToBeRecordedXP(pELR->EventID))
						{
							free(pELR);
							pELR=NULL;
							continue;
						}
						cat=EventMapXP[pELR->EventID];
					}
					else if(os=="VISTA"||os=="WIN7")
					{
						
						if(!EventToBeRecordedVISTA(pELR->EventID))
						{
							free(pELR);
							pELR=NULL;
							continue;
						}
						cat=EventMapVISTA[pELR->EventID];
					}
                    TCHAR *pValue;
					
					// This one is easy, the source record is right after the EVENTLOGRECORD struct
					pwzSource = (LPWSTR)((LPBYTE)pELR + sizeof(EVENTLOGRECORD));
					
                    pValue = (TCHAR *)pwzSource;
                    //pValue = pValue + ((_tcslen((const TCHAR *)pwzSource) + 1) * sizeof(TCHAR));
					// This one is a little more difficult, as it sits after the wide null at the end of pwzSource
					pwzComputer = (LPWSTR)pValue;

                    // create an entry for this event.
					repository.OpenNode(const_cast<char *>((to_utf8(cat)).c_str()));
                    repository.OpenNodeWithAttributes("Event");
                    _stprintf_s(szValue, sizeof(szValue)/sizeof(TCHAR), _T("%d"), pELR->RecordNumber);                    
                    repository.WriteAttribute("record", szValue);
                    repository.Write(">");

                    repository.WriteNodeWithValue("Log", ptszLog);
                    _stprintf_s(szValue, sizeof(szValue)/sizeof(TCHAR), _T("%d"), pELR->EventID);
					
                    repository.WriteNodeWithValue("RawEventID", szValue);
                    _stprintf_s(szValue, sizeof(szValue)/sizeof(TCHAR), _T("%d"), LOWORD(pELR->EventID));                    
                    repository.WriteNodeWithValue("EventID", szValue);
                    switch (pELR->EventType)
                    {
                    case EVENTLOG_ERROR_TYPE:
                        repository.WriteNodeWithValue("EventType", "Error");
                        break;
                    case EVENTLOG_AUDIT_FAILURE:
                        repository.WriteNodeWithValue("EventType", "Failure");
                        break;
                    case EVENTLOG_AUDIT_SUCCESS:
                        repository.WriteNodeWithValue("EventType", "Success");
                        break;
                    case EVENTLOG_INFORMATION_TYPE:
                        repository.WriteNodeWithValue("EventType", "Information");
                        break;
                    case EVENTLOG_WARNING_TYPE:
                        repository.WriteNodeWithValue("EventType", "Warning");
                        break;
                    default:
                        repository.WriteNodeWithValue("EventType", "Unknown");
                        break;
                    }
                    // convert the event time to local, machine time
                    time_t tempTime;
                    tempTime = pELR->TimeGenerated;
                    _tctime_s(szValue, sizeof(szValue)/sizeof(TCHAR),&tempTime);
                    repository.WriteNodeWithValue("EventTime", szValue);
                    repository.WriteNodeWithValue("EventComputer", (const TCHAR *)pwzComputer);
                    repository.WriteNodeWithValue("EventSource", (const TCHAR *)pwzSource);

			        // Get the actual username of the account creating this event if any
			        if(pELR->UserSidLength)
			        {
				        dwUserNameLen = MAX_PATH;
				        dwDomainNameLen = MAX_PATH;
						INC_SYS_CALL_COUNT(LookupAccountSid); // Needed to be INC TKH
				        if(LookupAccountSid(NULL, (PSID)((LPBYTE)pELR + pELR->UserSidOffset), tszUserName, &dwUserNameLen, tszDomainName, &dwDomainNameLen, &SNU))
				        {
                            repository.OpenNode("UserSID");
                            repository.Write(tszDomainName);
                            repository.Write("\\");
                            repository.Write(tszUserName);
                            repository.CloseNode("UserSID");
				        }
			        }

					

					
					// Check to see if we have to worry about inserts
					/*
					if(pELR->NumStrings > 0)
					{
     					szMessageBuffer = _T("");

						// Make our life easier and get the string offsets
                        // NOTE: this is being changed from wchar to char since the
                        // strings seem to be in char format.  This will need to be tested on some other
                        // system to determine if they are always chars.
						pszStrings = ((TCHAR *)(pELR) + (pELR->StringOffset));
						

						// Set up an array of DWORD_PTR's for FormatMessage
						for(DWORD dwpIdx = 0; dwpIdx < pELR->NumStrings; dwpIdx++)
						{
                            szMessageBuffer = szMessageBuffer + pszStrings;
                            szMessageBuffer = szMessageBuffer + _T("\n");
                            if (dwpIdx < (pELR->NumStrings-1))
                            {
							   pszStrings += _tcslen(pszStrings) + 1;	// walk to the next insert string
                            }
						}					
						
			            // Need to escape the xml in here as it may contain invalid characters...
                        repository.OpenNode("EventText");
                        if (pELR->EventID == 11707)
                        {
            			szMessageBuffer = XMLDelimitString(szMessageBuffer);

                        }
                        else
                        {
            			  szMessageBuffer = XMLDelimitString(szMessageBuffer);
                        }
                        repository.Write((to_utf8(szMessageBuffer)).c_str());
                        repository.CloseNode("EventText");
					} // end if (Insert strings)
					else
					{
						pDWPStrings = NULL;
					}
					*/
					//Uncommmet and debug above to get the eventText


			        // Dump any "user data" associated with this one
			        if(pELR->DataLength > 0)
			        {
                        repository.OpenNode("EventData");
				        for(dwTempIdx = 0; dwTempIdx < pELR->DataLength; dwTempIdx++)
				        {
                            _stprintf_s(szValue, sizeof(szValue)/sizeof(TCHAR), _T("%02x "), *((LPBYTE)pELR + pELR->DataOffset + dwTempIdx));
                            repository.Write(szValue);
				        }
                        repository.CloseNode("EventData");
			        }
                    repository.CloseNode("Event");

				} // end if (ReadEventLog)
			} // end if (ERROR_INSUFFICIENT_BUFFER)
			free(pELR);
			pELR = NULL;

			repository.CloseNode(const_cast<char *>((to_utf8(cat)).c_str()));
		}	// End ReadEventLog
	} // end for

	// Yay, we're done!
	CloseEventLog(hEventLog);

	return dwNumRecords;
}

int EventLogParser::GetEntryCount(LPCTSTR ptszLog)
{
	// This should suffice for pretty much any situation.
	HANDLE hEventLog = NULL;
	DWORD dwNumRecords = 0;

	if(!ptszLog)
		return 0;

	hEventLog = OpenEventLog(NULL, ptszLog);

	if(hEventLog == NULL)
		return 0;

	GetNumberOfEventLogRecords(hEventLog, &dwNumRecords);
	CloseEventLog(hEventLog);

	return dwNumRecords;
}

int EventLogParser::StartLog(LPCTSTR ptszFileName)
{
	_tfopen_s(&pFileHandle, ptszFileName, _T("w"));

	if(!pFileHandle)
	{
		return 0;
	}
	else
	{
		_ftprintf(pFileHandle, _T("<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n"));
		_ftprintf(pFileHandle, _T("<EventLog>\n"));
	}

	return 1;
}

int EventLogParser::CloseLog(void)
{
	if(pFileHandle)
	{
		_ftprintf(pFileHandle, _T("</EventLog>\n"));
		fclose(pFileHandle);
	}
	
	return 1;
}


#if 0					
					// This is where life would be easier if we were doing this all in memory
					// but we're not.  Oh well.  First we build a path to the registry key for the source...
					_stprintf_s(tszEventSource, MAX_PATH, _T("%s\\%s\\%s"), EVENTLOG_ROOT, ptszLog, pwzSource);
					// If we can open the registry key for this source we're golden, otherwise screwed we are.
					INC_SYS_CALL_COUNT(RegOpenKeyEx); // Needed to be INC TKH
					if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, tszEventSource, 0, KEY_READ, &hkMsgSource) == ERROR_SUCCESS)
					{
						dwMsgFileLen = MAX_PATH;
						dwRegKeyType = 0;

						INC_SYS_CALL_COUNT(RegQueryValueEx); // Needed to be INC TKH
						if(RegQueryValueEx(hkMsgSource, MESSAGEFILE_KEY, NULL, &dwRegKeyType, (LPBYTE)tszEventSource, &dwMsgFileLen) == ERROR_SUCCESS)
						{
							// Expand out the environment strings
							if(dwRegKeyType == REG_EXPAND_SZ)
							{
								// Reuse tszEventSource
								ExpandEnvironmentStrings(tszEventSource, tszMessageFile, MAX_PATH);
							}
							else
							{
								_tcscpy_s(tszMessageFile, MAX_PATH, tszEventSource);
							}
						}

						// We're done with the registry stuff
						INC_SYS_CALL_COUNT(RegCloseKey); // Needed to be INC TKH
						RegCloseKey(hkMsgSource);

						// Now we can load the message DLL(s) and parse out the strings.
						ptzToken = _tcstok_s(tszMessageFile, _T(";"), &ptzNextToken);

						while(ptzToken != NULL)
						{
							INC_SYS_CALL_COUNT(LoadLibraryEx); // Needed to be INC TKH
							hMsgDLL = LoadLibraryEx(ptzToken, NULL, LOAD_LIBRARY_AS_DATAFILE);

							if(hMsgDLL)
							{
								// Should probably do some "goto" crap for code failures here but that makes too 
								// much sense.  I never thought I'd see the day where using goto's would actually make sense.

								// pDWPStrings will be NULL if there's no inserts (see above)
								try {
									dwRetVal = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | 
														FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
														hMsgDLL, pELR->EventID, 0, (LPTSTR)&ptzMessageBuffer, MAX_PATH, (va_list*)pDWPStrings);
								}
								catch(...) {
									printf("FormatMessage Error?\n");
									dwRetVal = 0;
								}

								INC_SYS_CALL_COUNT(FreeLibrary); // Needed to be INC TKH
								FreeLibrary(hMsgDLL);

								if(dwRetVal)
									break;	// We found our message... BREAK!!!
								else
								{
									// This will be called if we can't find a message string in a particular DLL.
									// Mostly this should only occur if we have multiple message DLLs...

									if(ptzMessageBuffer)
									{
										INC_SYS_CALL_COUNT(LocalFree); // Needed to be INC TKH
										LocalFree(ptzMessageBuffer);
									}

									ptzMessageBuffer = NULL;
								}
							}
							else
							{
								dwRetVal = 0;
							}

							ptzToken = _tcstok_s(NULL, _T(";"), &ptzNextToken);
						}
					} // end if (Registry - Event Message File)
#endif

#undef _UNICODE
#undef UNICODE