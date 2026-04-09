#pragma once
#include "stdafx.h"
#include <map>
#include <vector>
#include <string>
#include "tstl.h"

using namespace std;

typedef enum _ENUM_PipeState {
	DISCONNECTED,
	WAITING_FOR_CONNECT, 
	CONNECTED, 
	WAITING_FOR_READ, 
	READ_COMPLETE,
	COMM_FAILURE
} PipeState;

typedef enum _ENUM_ScanState {
	UNKNOWN_STATE, 
	INITIALIZING,
	READING_SCANCONFIG,
	HASHING, 
	CAPTURING_SYSDATA,
	CAPTURING_VMEM,
	CAPTURING_PMEM,
	ABORTING, 
	SCAN_COMPLETE, 
	SCAN_FAILED,
	CAPTURING_DRIVEINFO,
	CAPTURING_EVENTS,
	CAPTURING_NETWORK,
	CAPTURING_REGISTRY,
	CAPTURING_INSTALLEDAPPS,
	ENCRYPTING,
	COPYING_FILES,
	DISK_FULL,
	CAPTURING_RECENT_FILES,
	CAPTURE_COMPLETE,
#ifdef RECENTDOCS
	CAPTURING_RECENTDOCS,
#endif
#ifdef TRUECRYPT
	IMAGING_START,
	IMAGING_FAILED,
	IMAGING_COMPLETE,
#endif
#ifdef MAILFILECAPTURE
	MAIL_START,
	MAIL_COMPLETE,
#endif
	ABORTED
} ScanState;
 
typedef unsigned int FileHashType;
static const FileHashType HASHTYPE_NOFILES = 0;
static const FileHashType HASHTYPE_CUSTOM = 1;
static const FileHashType HASHTYPE_LOGICAL = 2;
static const FileHashType HASHTYPE_REMOVABLE = 4;
static const FileHashType HASHTYPE_DESKTOP = 8;


typedef struct _STRUCT_ScanConfig {

	~_STRUCT_ScanConfig()
	{
		cout << "scan config destrcutor: @" << this << endl;
	}
	//(Rearranged) Variables declared in the order as they appear in the scanconfig file. It is easier this way

	//<System>
		bool getOperatingSystem;
		bool getMachineInformation;
		bool getAllUsers; //There is a function GetLoggedinUsers but the corresponding config variable is never used so I have commented it out.
		//<LOGS>
			//<Security> //Replaces getEvent
				bool getStartShutActions;
				bool getLoginLogoutActions;
				bool getAccountActions;
				bool getFirewallActions;
				bool getMiscActions;
			//</Security>
		//</LOGS>
	//</System>

	//<Hardware>
		bool getDrivers;
		bool getStorageDevices;
	//</Hardware>
	
	//<LiveFileSystems>
		bool getLiveFileSystems;
	//</LiveFileSystems>

	//<Network>
		bool getIPConfiguration; //This was p instead of P. I wonder how it was working before?
		bool getARPTable;
		bool getNetStat;
	//</Network>

	//<Processes>
		bool getGetProcesses;
	//</Processes>

	//<MemoryCapture>
		bool getPhysicalMemory;
	//</MemoryCapture>

	//<RecentlyUsedCapture>
		//<Enabled>
		//</Enabled>
		//<PreviousDays>
			bool registryMRU;
			int getPreviousDays;
			int modifiedDate;
			int createdDate;
		//</PreviousDays>
		//<MaximumFileSize>
			unsigned long long getMaximumFileSize;
		//</MaximumFileSize>
		//<Categories>
			//This is dynamic so use a map
			//<Documents>
					//<Extensions>

					//</Extensions>
			//</Documents>
			//<Images>
					//<Extensions>

					//</Extensions>
			//</Images>
			//<MultiMedia>
					//<Extensions>

					//</Extensions>
			//</MultiMedia>
		//</Categories>
		
			std::map<tstring,std::vector<tstring> > fileExtractionMap;
		//<SearchLocations>
			//<FixedMedia>
				bool getFixedMedia;
			//</FixedMedia>
			//<RemovableMedia>
				bool getRemovableMedia;
			//</RemovableMedia>
			//<NetworkMedia>
				bool getNetworkMedia;
			//</NetworkMedia>
			//<LiveFileSystem>
				bool getLiveFileSystem;
			//</LiveFileSystem>
		//</SearchLocations>
	//</RecentlyUsedCapture>

	//<ScreenCapture>
		//<FullScreen>
			bool getFullScreen;
		//</FullScreen>
		//<Desktop>
			bool getDesktop;
		//</Desktop>
		//<IndividualWindows>
			bool getIndividualWindows;
		//</IndividualWindows>
		//<BrowserTabs>
				bool getBrowserTabs;
		//</BrowserTabs>
	//</ScreenCapture>
	//<Services>
		//<ServiceStatus>
			bool getServiceStatus;
		//</ServiceStatus>
	//</Services>
	//<InstalledApps>
		bool getApplication; //This variable is named wrongly. I won't correct it now since it would require looking at other places.
	//</InstalledApps>
	//<Registry>
		bool getFolderSearch;
		bool getFileSearch;
		bool getRecentlyRun;
		bool getRecentDocs;
		//std::vector<std::tstring> MapRegistryKey; No longer used, remove after 1.2.5
	//</Registry>

#ifdef RECENTDOCS
/* Capturing My Recent Documents 
	<MyRecentDocuments>
		<Enabled>1</Enabled>
		<Users>
			<Active>1</Active>
			<All>1</All>
		</Users>
		<Target>
			<FollowNetwork>1</FollowNetwork>
			<FollowLocal>1</FollowLocal>
			<FollowRemovable>1</FollowRemovable>
		</Target>
  </MyRecentDocuments>
  */
		/* My Recent Documents capture*/
		bool getMRD;
		bool getrdActive;
		bool getrdAll;
		bool getFollowNetwork;
		bool getFollowLocal;
		bool getFollowRemovable;
		/* My Recent Documents capture*/
#endif 

#ifdef TRUECRYPT
		bool getAcquireTruecryptImage; //For dd imaging
#endif
#ifdef MAILFILECAPTURE
		bool getCaptureMailFiles; // Capturing mail files 
#endif

#ifdef WEBFORENSICS
		bool getWebHistory;
#endif

#ifdef TIMELINE
		bool getActionTimeLine;
#endif

	bool getSysData;//--Another stupidity.

} ScanConfig;