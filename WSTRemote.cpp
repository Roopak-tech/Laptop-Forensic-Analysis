// WSTRemote.cpp : Defines the entry point for the console application.
//

#define _UNICODE
#define UNICODE


#ifdef USBHISTORY
#pragma comment(linker, "/defaultlib:USBInfoExtract.lib")
#endif
  
#pragma warning(disable: 4996)
//#include "winerror.h"
#include "stdafx.h"
#include "WSTRemote.h"
#include "EnumServices.h"
#include "EventLogParser.h"
#include "NetworkInfo.h"
#include "DiskVolume.h"
#include "DriveSerial.h"
#include "WSTLog.h"
#include "RegParse.h"
#include "SysCallsLog.h"
#include <stdio.h>
#include <fcntl.h>
//#include <wlanapi.h>
#include <string>
#include "tstl.h"
#include <stdlib.h>
#include "WSTCommon.h"
#include <windows.h>

/* Let us use USE_WINPMEM driver .exe directly */
#define USE_WPMEM

#ifdef PRO
extern TCHAR caseDrive[];
extern TCHAR uslattDrive[];
#endif
extern int utfchecker(string,string);
extern void SecondsSince1970(DWORD,string &);
extern string to_utf8(const wstring &);
extern int DumpMemory64();

extern void validateXMLFile(const char*,const char*);	
extern memory_map *memory_region_file_map; // Physical memory region -> physmemX.bin mapping to be used in the results.xml

extern TCHAR caseDrive[10];
extern TCHAR uslattDrive[4];

#ifdef USBHISTORY
int GetUSBDeviceHistoryFromMachine(HashedXMLFile &repository);
#endif

//Globals
//list<char> trueCryptDrives;
//tstring globalCaseDir;
	
	

	int config_MAX_TAB_COUNT;
	int config_SLEEP_SECONDS;
	int config_MRU_TIME;

	typedef map<tstring, vector<tstring> > Mapvec;

	bool process_registry;//I should move it into the WSTRemote class later on.
	bool process_eventlogs;//I should move it into the WSTRemote class later on.
	bool process_filecapture;
	bool allFiles;
	


	char *g_szBuses[BusTypeMax] = {"unknown", "SCSI", "ATAPI", "ATA", "1394", "SSA", "Fibre", "USB", "RAID", "SCSI",
                     "SAS", "SATA", "SD", "MMC","Virtual","FileBackedVirtual","Spaces","Nvme","SCM","Ufs"};

	//vector<tstring> removable,network,fixed_drive,lfs;
struct hashStruct
{
	UCHAR hash[20];
	ULONG StatusCode;
}hashStruct;

struct inputStruct
{	
	HANDLE phymemFile;  //Pass in the handle to the file explicitly instead of the filename
	HANDLE pEvent; // Percent complete event.
	int PERCENT; //Indicates the resolution of percent completion reported.
	int SPEEDUP; //Indicates number of blocks captured before the write to USB drive.
}inputStruct;

HANDLE phymemFile = INVALID_HANDLE_VALUE; // To implement the abort 

string XMLDelimitString(string szSource);
string XMLDelimitString(const char *szValue);

// #pragma comment(lib, "tomcrypt.lib")

//#define _ENABLE_ENCRYPTION
//#define GEM_LIVEWIRE

//NLR version should use DES while the local version needs to use AES
//Changed the CIPHER to des encryption instead of aes. --RJM(04/08/2009.)
#ifndef NLR
	#define CIPHER "aes"
	#define CIPHER_DESC aes_desc
#else
	#define CIPHER "des"
	#define CIPHER_DESC des_desc
#endif // NLR

void AddStructToXML(HashedXMLFile &repository,int i, LPNETRESOURCE lpnrLocal);
BOOL WINAPI EnumerateFunc(HashedXMLFile &repository,LPNETRESOURCE lpnr);

// global variables used to reduce dynamic allocations.  
TCHAR g_tempBuf[256];  // use only locally and inline!! It could be overwritten on sub routine call.
OSVERSIONINFOEX g_osvex;


#ifdef _DEBUG
void WSTRemote::OutputConfig()
{

	FILE *fd=fopen("scanconfig_read.txt","w+");
	//<System>
		 fprintf(fd,"OperatingSystem %d\n",config.getOperatingSystem);
		 fprintf(fd,"MachineInformation %d\n",config.getMachineInformation);
		 fprintf(fd,"Users %d\n",config.getAllUsers); //There is a function GetLoggedinUsers but the corresponding config variable is never used so I have commented it out.
		//<LOGS>
			//<Security> //Replaces fprintf(fd,"%d\n",config.getEvent
				 fprintf(fd,"StartShutActions %d\n",config.getStartShutActions);
				 fprintf(fd,"LoginLogoutActions %d\n",config.getLoginLogoutActions);
				 fprintf(fd,"AccountActions %d\n",config.getAccountActions);
				 fprintf(fd,"FirewallActions %d\n",config.getFirewallActions);
				 fprintf(fd,"MiscActions %d\n",config.getMiscActions);
			//</Security>
		//</LOGS>
	//</System>

	//<Hardware>
		 fprintf(fd,"Drivers %d\n",config.getDrivers);
		 fprintf(fd,"StorageDevices %d\n",config.getStorageDevices);
	//</Hardware>
	
	//<LiveFileSystems>
		 fprintf(fd,"LiveFileSystems %d\n",config.getLiveFileSystems);
	//</LiveFileSystems>

	//<Network>
		 fprintf(fd,"IPConfiguration %d\n",config.getIPConfiguration); //This was p instead of P. I wonder how it was working before?
		 fprintf(fd,"ARPTable %d\n",config.getARPTable);
		 fprintf(fd,"NetStat %d\n",config.getNetStat);
	//</Network>

	//<Processes>
		 fprintf(fd,"GetProcesses %d\n",config.getGetProcesses);
	//</Processes>

	//<MemoryCapture>
		 fprintf(fd,"PhysicalMemory %d\n",config.getPhysicalMemory);
	//</MemoryCapture>

	//<RecentlyUsedCapture>
		//<PreviousDays>
			 fprintf(fd,"PreviousDays %d\n",config.getPreviousDays);
		//</PreviousDays>
		//<MaximumFileSize>
			 fprintf(fd,"MaximumFileSize %d\n",config.getMaximumFileSize);
		//</MaximumFileSize>
		//<Categories>
			//<Documents>
				// fprintf(fd,"Documents %d\n",config.getDocuments);
			//</Documents>
			//<Images>
				// fprintf(fd,"Images %d\n",config.getImages);
			//</Images>
			//<MultiMedia>
				// fprintf(fd,"MultiMedia %d\n",config.getMultiMedia);
			//</MultiMedia>
		//</Categories>
		//<Extensions>
			//<doc>
				// fprintf(fd,"doc %d\n",config.getdoc);
			//</doc>
			//<docx>
			//	 fprintf(fd,"docx %d\n",config.getdocx);
			//</docx>
			//<wav>
			//	 fprintf(fd,"wav %d\n",config.getwav);
			//</wav>
		//</Extensions>
		//<SearchLocations>
			//<FixedMedia>
				 fprintf(fd,"FixedMedia %d\n",config.getFixedMedia);
			//</FixedMedia>
			//<RemovableMedia>
				 fprintf(fd,"RemovableMedia %d\n",config.getRemovableMedia);
			//</RemovableMedia>
			//<NetworkMedia>
				 fprintf(fd,"NetworkMedia %d\n",config.getNetworkMedia);
			//</NetworkMedia>
				 //<NetworkMedia>
				 fprintf(fd,"LiveFileSystem %d\n",config.getLiveFileSystem);
			//</NetworkMedia>
		//</SearchLocations>
	//</RecentlyUsedCapture>

	//<ScreenCapture>
		//<FullScreen>
			 fprintf(fd,"FullScreen %d\n",config.getFullScreen);
		//</FullScreen>
		//<Desktop>
			 fprintf(fd,"Desktop %d\n",config.getDesktop);
		//</Desktop>
		//<IndividualWindows>
			 fprintf(fd,"IndividualWindows %d\n",config.getIndividualWindows);
		//</IndividualWindows>
		//<BrowserTabs>
				 //fprintf(fd,"BrowserTabs %d\n",config.getBrowserTabs);
		//</BrowserTabs>
	//</ScreenCapture>
	//<Services>
		//<ServiceStatus>
			 fprintf(fd,"ServiceStatus %d\n",config.getServiceStatus);
		//</ServiceStatus>
	//</Services>
	//<InstalledApps>
		 fprintf(fd,"Application %d\n",config.getApplication); //This variable is named wrongly. I won't correct it now since it would require looking at other places.
	//</InstalledApps>
	//<Registry>
		 /*
		//<OpenSaveMRU>
			 fprintf(fd,"OpenSaveMRU %d\n",config.getOpenSaveMRU);
		//</OpenSaveMRU>
		//<RunMRU>
			 fprintf(fd,"RunMRU %d\n",config.getRunMRU);
		//</RunMRU>
		//<LastVisitedMRU>
			 fprintf(fd,"LastVisited %d\n",config.getLastVisitedMRU);
		//</LastVisitedMRU>
		//<RecentDocs>
			 fprintf(fd,"RecentDocs %d\n",config.getRecentDocs);
		//</RecentDocs>
		//<FolderSearchTerms>
			 fprintf(fd,"FolderSearchTerms %d\n",config.getFolderSearchTerms);
		//</FolderSearchTerms>
		//<FileSearchTerms>
			 fprintf(fd,"FileSearchTerms %d\n",config.getFileSearchTerms);
		//</FileSearchTerms>
		//<Wireless>
			 fprintf(fd,"Wireless %d\n",config.getWireless);
		//</Wireless>
		*/
	//</Registry>

	 fprintf(fd,"SysData %d\n",config.getSysData);//--Another stupidity.

#ifdef RECENTDOCS
		fprintf(fd,"RDActive %d\n",config.getrdActive);
		fprintf(fd,"RDAll %d\n",config.getrdAll);
		fprintf(fd,"FollowNetwork %d\n",config.getFollowNetwork);
		fprintf(fd,"FollowLocal %d\n",config.getFollowLocal);
		fprintf(fd,"FollowRemovable %d\n",config.getFollowRemovable);
#endif
#ifdef TRUECRYPT
		fprintf(fd,"TrueCryptImaging %d\n",config.getAcquireTruecryptImage);
#endif
#ifdef MAILFILECAPTURE
		fprintf(fd,"MailFileCapture %d\n",config.getCaptureMailFiles);
#endif

	 fclose(fd);
}

#endif


// Declaration of the commom resources structrue 
//WSTCommon commonResources;

//
//  WSTRemote Constructor
//
//
WSTRemote::WSTRemote(tstring xmlConfigFilename) 
{
//	cout<<CIPHER<<endl;
#ifdef NLR
//    AppLog.Write("Using DES for encryption", WSLMSG_STATUS);
#else
//    AppLog.Write("Using AES for encryption", WSLMSG_STATUS);
#endif

    filesProcessed = 0;
    procsCaptured = 0;
    iUseEncryption = 0;
    m_bHaveMemory = false;

//    AppLog.Write("Initializing system.", WSLMSG_STATUS);

    // set configuration defaults
    ResetConfig();

    // load config file
    if(LoadConfigFile(xmlConfigFilename))
    {
    	gui.DisplayStatus(_T("Config read success.."));
	
#ifdef _DEBUG
		OutputConfig();
#endif

	}
	else 
	{
		gui.DisplayStatus(_T("Config status Read Failure"));
	}	

	long long availSpace=0;
#ifdef PRO
	GetDiskFreeSpaceEx(caseDrive,(PULARGE_INTEGER)&availSpace,NULL,NULL);
#else
	GetDiskFreeSpaceEx(NULL,(PULARGE_INTEGER)&availSpace,NULL,NULL);
#endif

	
	if(availSpace <= MINIMUM_SPACE_REQUIRED )
	{
		currentState = DISK_FULL;
	}

	config_MAX_TAB_COUNT=50;
	config_SLEEP_SECONDS=2000;
	
}

/* 
* checking windows version 64/32bit to run the 
* physical memory capture
*/
BOOL Is64BitWindows()
{
	SYSTEM_INFO systeminfo;
	GetSystemInfo(&systeminfo);
	if(systeminfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
		return TRUE;
	else 
		return TRUE;	
	
}



int InitializeCaseDirectory(tstring dirName)
{

	INC_SYS_CALL_COUNT(SetCurrentDirectory); 
	if(!SetCurrentDirectory(dirName.c_str())) //First set the current directory to newly created case dir.
	{
		AppLog.Write("SetCurrentDirectory Failed");
		return -1;
	}

	INC_SYS_CALL_COUNT(CreateDirectory); 

	BOOL createStatus=CreateDirectory(_T("TriageResults"),NULL); //Create TriageResults
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("TriageResults Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - TriageResults");
			return -3;
		}
	}

	INC_SYS_CALL_COUNT(CreateDirectory); 

	createStatus=CreateDirectory(_T("Screenshots"),NULL); //Create Screenshots 
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("Screenshots Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - Screenshots");
			return -3;
		}
	}

	INC_SYS_CALL_COUNT(CreateDirectory); 

	createStatus=CreateDirectory(_T("Setup"),NULL); //Create Setup
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("Setup Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - Setup");
			return -3;
		}
	}



	//Added additional directory called Analysis -- 02/16/2011 RJM
	INC_SYS_CALL_COUNT(CreateDirectory); 

	createStatus=CreateDirectory(_T("Analysis"),NULL); //Where analysis files go
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("Analysis Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - Analysis");
			return -3;
		}
	}

	//Added additional directory called Reports -- 02/16/2011 RJM
	INC_SYS_CALL_COUNT(CreateDirectory); 

	createStatus=CreateDirectory(_T("Reports"),NULL); //Where report files go
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("Reports Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - Reports");
			return -3;
		}
	}

	INC_SYS_CALL_COUNT(CreateDirectory); 

	createStatus=CreateDirectory(_T("Forensic"),NULL); //Create Forensic
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("Forensic Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - Forensic");
			return -3;
		}
	}

	INC_SYS_CALL_COUNT(CreateDirectory); 

	createStatus=CreateDirectory(_T("Files"),NULL); //Create Files
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("Files Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - Files");
			return -3;
		}
	}

	INC_SYS_CALL_COUNT(SetCurrentDirectory); 
	if(!SetCurrentDirectory(_T("Files"))) //To create sub-directories
	{
		AppLog.Write("SetCurrentDirectory - Files Failed");
		return -1;
	}

	INC_SYS_CALL_COUNT(CreateDirectory); 

	createStatus=CreateDirectory(_T("MRU"),NULL); //Create MRU
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("MRU Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - MRU");
			return -3;
		}
	}
	INC_SYS_CALL_COUNT(CreateDirectory); // Needed to be INC TKH
	createStatus=CreateDirectory(_T("MRD"),NULL); //Create MRD
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("MRD Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - MRD");
			return -3;
		}
	}
	INC_SYS_CALL_COUNT(CreateDirectory); // Needed to be INC TKH
	createStatus=CreateDirectory(_T("Collected"),NULL); //Create Files
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("Collected Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - Collected");
			return -3;
		}
	}
#ifdef WEBFORENSICS

	INC_SYS_CALL_COUNT(CreateDirectory); // Needed to be INC TKH
	createStatus=CreateDirectory(_T("Web"),NULL); //Create Files
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("Web Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - Web");
			return -3;
		}
	}
#endif
#ifdef TIMELINE

	INC_SYS_CALL_COUNT(CreateDirectory); // Needed to be INC TKH
	createStatus=CreateDirectory(_T("Timeline"),NULL); //Create Files
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("Timeline Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - Timeline");
			return -3;
		}
	}
#endif
#ifdef MAILFILECAPTURE
	INC_SYS_CALL_COUNT(CreateDirectory);
	createStatus=CreateDirectory(_T("MailFiles"),NULL); //Create Files
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("MailFiles Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - MailFiles");
			return -3;
		}
	}
#endif
	INC_SYS_CALL_COUNT(SetCurrentDirectory); 
	if(!SetCurrentDirectory(_T(".."))) //First set the current directory to newly created case dir.
	{
		AppLog.Write("SetCurrentDirectory Failed");
		return -1;
	}

#ifdef DRIVE_IMAGING 
	INC_SYS_CALL_COUNT(CreateDirectory); // Needed to be INC TKH
	createStatus=CreateDirectory(_T("Drive Images"),NULL); //Create MRD
	if(!createStatus)
	{
		if(GetLastError() == ERROR_ALREADY_EXISTS)
		{
			AppLog.Write("Drive Images Directory already exists. Try running the acquisition again");
			return -2;
		}
		else
		{
			AppLog.Write("Error creating directory - Drive Images");
			return -3;
		}
	}
#endif
	return 0;
}

#ifdef PRO
int CopyAuxFiles(tstring caseName,tstring lattDrive)
{

	
	TCHAR dirName[MAX_PATH];
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH,dirName);
	

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\ScanConfig.xml"))).c_str(),(tstring(dirName)+_T("\\Setup\\ScanConfig.xml")).c_str(),true))
    {
 	    AppLog.Write("Scanconfig.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("Scanconfig.xml CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\filedigest.xslt"))).c_str(),(tstring(dirName)+_T("\\Files\\Collected\\filedigest.xslt")).c_str(),true))
    {
 	    AppLog.Write("filedigest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("filedigest.xslt CopyFile Failed"));
		
	    //return -1;
    }
#ifdef MAILFILECAPTURE
	INC_SYS_CALL_COUNT(CopyFile); 
	if(!CopyFile((lattDrive+tstring(_T("Program\\filedigest.xslt"))).c_str(),(tstring(dirName)+_T("\\Files\\MailFiles\\filedigest.xslt")).c_str(),true))
    {
 	    AppLog.Write("filedigest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("filedigest.xslt CopyFile Failed"));
	    //return -1;
    }
#endif
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\filedigest.xslt"))).c_str(),(tstring(dirName)+_T("\\Files\\MRU\\filedigest.xslt")).c_str(),true))
    {
 	    AppLog.Write("filedigest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("filedigest.xslt CopyFile Failed"));
		
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\filedigest.xslt"))).c_str(),(tstring(dirName)+_T("\\Files\\MRD\\filedigest.xslt")).c_str(),true))
    {
 	    AppLog.Write("filedigest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("filedigest.xslt CopyFile Failed"));
		
	    //return -1;
    }

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\filedigest.xslt"))).c_str(),(tstring(dirName)+_T("\\Files\\Web\\filedigest.xslt")).c_str(),true))
    {
 	    AppLog.Write("filedigest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("filedigest.xslt CopyFile Failed"));
		
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\filedigest.xslt"))).c_str(),(tstring(dirName)+_T("\\Files\\Timeline\\filedigest.xslt")).c_str(),true))
    {
 	    AppLog.Write("filedigest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("filedigest.xslt CopyFile Failed"));
		
	    //return -1;
    }
#ifdef WEBFORENSICS
INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\\WebForensics.xslt"))).c_str(),(tstring(dirName)+_T("\\Files\\Web\\WebForensics.xslt")).c_str(),true))
    {
 	    AppLog.Write("WebForensics.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("WebForensics.xslt CopyFile Failed"));
		
	    //return -1;
    }

#endif
#ifdef TIMELINE
INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\\EventRecords.xslt"))).c_str(),(tstring(dirName)+_T("\\Files\\Timeline\\EventRecords.xslt")).c_str(),true))
    {
 	    AppLog.Write("EventRecords.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("EventRecords.xslt CopyFile Failed"));
		
	    //return -1;
    }

#endif
		
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\ScanConfig.xslt"))).c_str(),(tstring(dirName)+_T("\\Setup\\ScanConfig.xslt")).c_str(),true))
    {
	    AppLog.Write("Scanconfig.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("Scanconfig.xslt CopyFile Failed"));
	    //return -1;
    }

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\results.xslt"))).c_str(),(tstring(dirName)+_T("\\TriageResults\\results.xslt")).c_str(),true))
    {
	    AppLog.Write("results.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("results.xslt CopyFile Failed"));
	    //return -1;
    }

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\image.jpg"))).c_str(),(tstring(dirName)+_T("\\TriageResults\\image.jpg")).c_str(),true))
    {
	    AppLog.Write("image.jpg CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("image.jpg CopyFile Failed"));
	    
    }

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\screenshot_digest.xslt"))).c_str(),(tstring(dirName)+_T("\\Screenshots\\screenshot_digest.xslt")).c_str(),true))
    {
	    AppLog.Write("screenshot_digest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("screenshot_digest.xslt CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\digests.xslt"))).c_str(),(tstring(dirName)+_T("\\Forensic\\digests.xslt")).c_str(),true))
    {
	    AppLog.Write("digests.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("digests.xslt CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\scanlog.xslt"))).c_str(),(tstring(dirName)+_T("\\Forensic\\scanlog.xslt")).c_str(),true))
    {
	    AppLog.Write("scanlog.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("scanlog.xslt CopyFile Failed"));

	    //return -1;
    }
	
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\sigverify.exe"))).c_str(),(tstring(dirName)+_T("\\Forensic\\sigverify.exe")).c_str(),true))
    {
	    AppLog.Write("sigverify.exe CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("sigverify.exe CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\sigverify.xslt"))).c_str(),(tstring(dirName)+_T("\\Forensic\\sigverify.xslt")).c_str(),true))
    {
	    AppLog.Write("sigverify.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("sigverify.xslt CopyFile Failed"));
	    //return -1;
    }
	
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\validationReport.xml"))).c_str(),(tstring(dirName)+_T("\\Forensic\\validationReport.xml")).c_str(),true))
    {
	    AppLog.Write("validationReport.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("validationReport.xml CopyFile Failed"));
	    //return -1;
    }
	
	//To copy the files reports.txt,analysis.txt and Analysis.exe -- 02/16/2011 RJM
	/*
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\\reports.txt"))).c_str(),(tstring(dirName)+_T("\\Reports\\reports.txt")).c_str(),true))
    {
 	    AppLog.Write("reports.txt CopyFile Failed",WSLMSG_STATUS); //We need not return if we fail.
		gui.DisplayWarning(_T("reports.txt CopyFile Failed"));
    }
	
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\\analysis.txt"))).c_str(),(tstring(dirName)+_T("\\Analysis\\analysis.txt")).c_str(),true))
    {
 	    AppLog.Write("analysis.txt CopyFile Failed",WSLMSG_STATUS); //We need not return if we fail.
		gui.DisplayWarning(_T("analysis.txt CopyFile Failed"));
    }
	*/
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\Analysis.exe"))).c_str(),(tstring(dirName)+_T("\\Analysis.exe")).c_str(),true))
    {
 	    AppLog.Write("Analysis.exe CopyFile Failed",WSLMSG_STATUS); //return -1 if we fail
		gui.DisplayWarning(_T("Analysis.exe CopyFile Failed"));
		return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\allimages.xslt"))).c_str(),(tstring(dirName)+_T("\\Reports\\allimages.xslt")).c_str(),true))
    {
 	    AppLog.Write("allimages.xslt CopyFile Failed",WSLMSG_STATUS); //return -1 if we fail
		gui.DisplayWarning(_T("allimages.xslt CopyFile Failed"));
    }
	//Removed index.html
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("\\.id"))).c_str(),(tstring(dirName)+_T("\\.id")).c_str(),true))
    {
	    AppLog.Write(".id CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T(".id CopyFile Failed"));
	    return -1;
    }

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\screenshot_digest.xml"))).c_str(),(tstring(dirName)+_T("\\Screenshots\\screenshot_digest.xml")).c_str(),true))
    {
	    AppLog.Write("screenshot_digest.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("screenshot_digest.xml CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\dummy_file.xml"))).c_str(),(tstring(dirName)+_T("\\Files\\Collected\\collected_files.xml")).c_str(),true))
    {
	    AppLog.Write("collected_files.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("collected_files.xml CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\\dummy_file.xml"))).c_str(),(tstring(dirName)+_T("\\Files\\MRD\\recentdocs.xml")).c_str(),true))
    {
	    AppLog.Write("recentdocs.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("recentdocs.xml CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\dummy_file.xml"))).c_str(),(tstring(dirName)+_T("\\Files\\MRU\\mrudigest.xml")).c_str(),true))
    {
	    AppLog.Write("mrudigest.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("mrudigest.xml CopyFile Failed"));
	    //return -1;
    } 
#ifdef MAILFILECAPTURE
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\dummy_file.xml"))).c_str(),(tstring(dirName)+_T("\\Files\\MailFiles\\MailFiles.xml")).c_str(),true))
    {
	    AppLog.Write("MailFiles.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("MailFiles.xml CopyFile Failed"));
	    //return -1;
    } 
#endif
#ifdef WEBFORENSICS
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\dummy_file.xml"))).c_str(),(tstring(dirName)+_T("\\Files\\Web\\WebForensics.xml")).c_str(),true))
    {
	    AppLog.Write("WebForensics.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("WebForensics.xml CopyFile Failed"));
	    //return -1;
    } 
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\dummy_file.xml"))).c_str(),(tstring(dirName)+_T("\\Files\\Web\\SkypeLog.xml")).c_str(),true))
    {
	    AppLog.Write("SkypeLog.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("SkypeLog..xml CopyFile Failed"));
	    //return -1;
    } 
#endif
#ifdef TIMELINE
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\dummy_file.xml"))).c_str(),(tstring(dirName)+_T("\\Files\\Timeline\\EventRecords.xml")).c_str(),true))
    {
	    AppLog.Write("EventRecords.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("EventRecords.xml CopyFile Failed"));
	    //return -1;
    } 
#endif
	//For SkypeForensics
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\skypeForensics.xslt"))).c_str(),(tstring(dirName)+_T("\\Files\\Web\\skypeForensics.xslt")).c_str(),true))
    {
	    AppLog.Write("SkypeForensics.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("SkypeForensics.xslt CopyFile Failed"));
	    //return -1;
    } 
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile((lattDrive+tstring(_T("Program\\skypeChat.xslt"))).c_str(),(tstring(dirName)+_T("\\Reports\\skypeChat.xslt")).c_str(),true))
    {
	    AppLog.Write("skypeChat.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("skypeChat.xslt CopyFile Failed"));
	    //return -1;
    } 

	//copying Reports file
	INC_SYS_CALL_COUNT(CopyFile);
	if (!CopyFile((lattDrive + tstring(_T("Program\\ProgramForensics.xml"))).c_str(), (tstring(dirName) + _T("\\Reports\\ProgramForensics.xml")).c_str(), true))
	{
		AppLog.Write("ProgramForensics.xml CopyFile Failed", WSLMSG_STATUS); //return -1 if we fail
		gui.DisplayWarning(_T("ProgramForensics.xml CopyFile Failed"));
	}
	INC_SYS_CALL_COUNT(CopyFile);
	if (!CopyFile((lattDrive + tstring(_T("Program\\ProgramForensics.xslt"))).c_str(), (tstring(dirName) + _T("\\Reports\\ProgramForensics.xslt")).c_str(), true))
	{
		AppLog.Write("ProgramForensics.xslt CopyFile Failed", WSLMSG_STATUS); //return -1 if we fail
		gui.DisplayWarning(_T("ProgramForensics.xslt CopyFile Failed"));
	}

	//Removed index_files directory.
	
	return 0;
}
#else
int CopyAuxFiles(tstring caseName)
{

	
	TCHAR dirName[MAX_PATH];
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH,dirName);
	

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\ScanConfig.xml"),(tstring(dirName)+_T("\\Setup\\ScanConfig.xml")).c_str(),true))
    {
 	    AppLog.Write("Scanconfig.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("Scanconfig.xml CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\filedigest.xslt"),(tstring(dirName)+_T("\\Files\\Collected\\filedigest.xslt")).c_str(),true))
    {
 	    AppLog.Write("filedigest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("filedigest.xslt CopyFile Failed"));
		
	    //return -1;
    }
#ifdef MAILFILECAPTURE
	INC_SYS_CALL_COUNT(CopyFile); 
	if(!CopyFile(_T("..\\..\\Program\\filedigest.xslt"),(tstring(dirName)+_T("\\Files\\MailFiles\\filedigest.xslt")).c_str(),true))
    {
 	    AppLog.Write("filedigest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("filedigest.xslt CopyFile Failed"));
	    //return -1;
    }
#endif
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\filedigest.xslt"),(tstring(dirName)+_T("\\Files\\MRU\\filedigest.xslt")).c_str(),true))
    {
 	    AppLog.Write("filedigest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("filedigest.xslt CopyFile Failed"));
		
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\filedigest.xslt"),(tstring(dirName)+_T("\\Files\\MRD\\filedigest.xslt")).c_str(),true))
    {
 	    AppLog.Write("filedigest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("filedigest.xslt CopyFile Failed"));
		
	    //return -1;
    }
/*#ifdef WEBFORENSICS
INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\WebForensics.xslt"),(tstring(dirName)+_T("\\Files\\Web\\WebForensics.xslt")).c_str(),true))
    {
 	    AppLog.Write("WebForensics.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("WebForensics.xslt CopyFile Failed"));
		
	    //return -1;
    }

#endif
	*/
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\ScanConfig.xslt"),(tstring(dirName)+_T("\\Setup\\ScanConfig.xslt")).c_str(),true))
    {
	    AppLog.Write("Scanconfig.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("Scanconfig.xslt CopyFile Failed"));
	    //return -1;
    }

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\results.xslt"),(tstring(dirName)+_T("\\TriageResults\\results.xslt")).c_str(),true))
    {
	    AppLog.Write("results.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("results.xslt CopyFile Failed"));
	    //return -1;
    }

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\image.jpg"),(tstring(dirName)+_T("\\TriageResults\\image.jpg")).c_str(),true))
    {
	    AppLog.Write("image.jpg CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("image.jpg CopyFile Failed"));
	    
    }

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\screenshot_digest.xslt"),(tstring(dirName)+_T("\\Screenshots\\screenshot_digest.xslt")).c_str(),true))
    {
	    AppLog.Write("screenshot_digest.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("screenshot_digest.xslt CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\digests.xslt"),(tstring(dirName)+_T("\\Forensic\\digests.xslt")).c_str(),true))
    {
	    AppLog.Write("digests.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("digests.xslt CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\scanlog.xslt"),(tstring(dirName)+_T("\\Forensic\\scanlog.xslt")).c_str(),true))
    {
	    AppLog.Write("scanlog.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("scanlog.xslt CopyFile Failed"));

	    //return -1;
    }
	
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\sigverify.exe"),(tstring(dirName)+_T("\\Forensic\\sigverify.exe")).c_str(),true))
    {
	    AppLog.Write("sigverify.exe CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("sigverify.exe CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\sigverify.xslt"),(tstring(dirName)+_T("\\Forensic\\sigverify.xslt")).c_str(),true))
    {
	    AppLog.Write("sigverify.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("sigverify.xslt CopyFile Failed"));
	    //return -1;
    }
	
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\validationReport.xml"),(tstring(dirName)+_T("\\Forensic\\validationReport.xml")).c_str(),true))
    {
	    AppLog.Write("validationReport.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("validationReport.xml CopyFile Failed"));
	    //return -1;
    }
	
	//To copy the files reports.txt,analysis.txt and Analysis.exe -- 02/16/2011 RJM
	/*
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\reports.txt"),(tstring(dirName)+_T("\\Reports\\reports.txt")).c_str(),true))
    {
 	    AppLog.Write("reports.txt CopyFile Failed",WSLMSG_STATUS); //We need not return if we fail.
		gui.DisplayWarning(_T("reports.txt CopyFile Failed"));
    }
	
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\analysis.txt"),(tstring(dirName)+_T("\\Analysis\\analysis.txt")).c_str(),true))
    {
 	    AppLog.Write("analysis.txt CopyFile Failed",WSLMSG_STATUS); //We need not return if we fail.
		gui.DisplayWarning(_T("analysis.txt CopyFile Failed"));
    }
	*/
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\Analysis.exe"),(tstring(dirName)+_T("\\Analysis.exe")).c_str(),true))
    {
 	    AppLog.Write("Analysis.exe CopyFile Failed",WSLMSG_STATUS); //return -1 if we fail
		gui.DisplayWarning(_T("Analysis.exe CopyFile Failed"));
		return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\allimages.xslt"),(tstring(dirName)+_T("\\Reports\\allimages.xslt")).c_str(),true))
    {
 	    AppLog.Write("allimages.xslt CopyFile Failed",WSLMSG_STATUS); //return -1 if we fail
		gui.DisplayWarning(_T("allimages.xslt CopyFile Failed"));
    }
	//copying Reports file
	INC_SYS_CALL_COUNT(CopyFile);
	if (!CopyFile(_T("..\\..\\Program\\ProgramForensics.xml"), (tstring(dirName) + _T("\\Reports\\ProgramForensics.xml")).c_str(), true))
	{
		AppLog.Write("ProgramForensics.xml CopyFile Failed", WSLMSG_STATUS); //return -1 if we fail
		gui.DisplayWarning(_T("ProgramForensics.xml CopyFile Failed"));
	}
	INC_SYS_CALL_COUNT(CopyFile);
	if (!CopyFile(_T("..\\..\\Program\\ProgramForensics.xslt"), (tstring(dirName) + _T("\\Reports\\ProgramForensics.xslt")).c_str(), true))
	{
		AppLog.Write("ProgramForensics.xslt CopyFile Failed", WSLMSG_STATUS); //return -1 if we fail
		gui.DisplayWarning(_T("ProgramForensics.xslt CopyFile Failed"));
	}
	//Removed index.html
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("\\.id"),(tstring(dirName)+_T("\\.id")).c_str(),true))
    {
	    AppLog.Write(".id CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T(".id CopyFile Failed"));
	    return -1;
    }

	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\screenshot_digest.xml"),(tstring(dirName)+_T("\\Screenshots\\screenshot_digest.xml")).c_str(),true))
    {
	    AppLog.Write("screenshot_digest.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("screenshot_digest.xml CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\dummy_file.xml"),(tstring(dirName)+_T("\\Files\\Collected\\collected_files.xml")).c_str(),true))
    {
	    AppLog.Write("collected_files.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("collected_files.xml CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\dummy_file.xml"),(tstring(dirName)+_T("\\Files\\MRD\\recentdocs.xml")).c_str(),true))
    {
	    AppLog.Write("recentdocs.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("recentdocs.xml CopyFile Failed"));
	    //return -1;
    }
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\dummy_file.xml"),(tstring(dirName)+_T("\\Files\\MRU\\mrudigest.xml")).c_str(),true))
    {
	    AppLog.Write("mrudigest.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("mrudigest.xml CopyFile Failed"));
	    //return -1;
    } 
#ifdef MAILFILECAPTURE
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\dummy_file.xml"),(tstring(dirName)+_T("\\Files\\MailFiles\\MailFiles.xml")).c_str(),true))
    {
	    AppLog.Write("MailFiles.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("MailFiles.xml CopyFile Failed"));
	    //return -1;
    } 
#endif
#ifdef WEBFORENSICS
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\dummy_file.xml"),(tstring(dirName)+_T("\\Files\\Web\\WebForensics.xml")).c_str(),true))
    {
	    AppLog.Write("WebForensics.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("WebForensics.xml CopyFile Failed"));
	    //return -1;
    } 
#endif
#ifdef TIMELINE
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\dummy_file.xml"),(tstring(dirName)+_T("\\Files\\Timeline\\EventRecords.xml")).c_str(),true))
    {
	    AppLog.Write("EventRecords.xml CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("EventRecords.xml CopyFile Failed"));
	    //return -1;
    } 
#endif
	//For SkypeForensics
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\skypeForensics.xslt"),(tstring(dirName)+_T("\\Files\\Web\\skypeForensics.xslt")).c_str(),true))
    {
	    AppLog.Write("SkypeForensics.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("SkypeForensics.xslt CopyFile Failed"));
	    //return -1;
    } 
	INC_SYS_CALL_COUNT(CopyFile); 
    if(!CopyFile(_T("..\\..\\Program\\skypeChat.xslt"),(tstring(dirName)+_T("\\Reports\\skypeChat.xslt")).c_str(),true))
    {
	    AppLog.Write("skypeChat.xslt CopyFile Failed",WSLMSG_STATUS);
		gui.DisplayWarning(_T("skypeChat.xslt CopyFile Failed"));
	    //return -1;
    } 
 
	//Removed index_files directory.
	
	return 0;
}
#endif


WSTRemote::~WSTRemote(void)
{
    cout << "~wstremote3" << endl;
    tstring file;	

    if (fsHash.is_open()) {
	fsHash.flush();
	fsHash.close();
    }

}

#ifdef PRO

int SetupLogsAndResults(tstring & m_szRepositoryFile,tstring OrigscanFile)
{
	tstring szFilename;
	memset(g_tempBuf,0,sizeof(g_tempBuf));
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH, g_tempBuf);
    szFilename = g_tempBuf;
    szFilename = szFilename + _T("\\Forensic\\AuditReport_incomplete.xml");


	INC_SYS_CALL_COUNT(MoveFile); 
	if(!MoveFile(OrigscanFile.c_str(),szFilename.c_str()))
    {
		AppLog.Write(tstring(_T("MoveFile failed on file AuditReport_incomplete.xml from ")+OrigscanFile+_T(" to ")+szFilename).c_str(),WSLMSG_ERROR);
	    return -1;
    }

    AppLog.SetLogFile(szFilename.c_str());
    m_szRepositoryFile = tstring(g_tempBuf);
    m_szRepositoryFile = m_szRepositoryFile+ _T("\\TriageResults\\results_incomplete.xml");

	return 0;
}

#else
int SetupLogsAndResults(tstring & m_szRepositoryFile,tstring OrigscanFile)
{
	tstring szFilename;
	memset(g_tempBuf,0,sizeof(g_tempBuf));
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH, g_tempBuf);
    szFilename = g_tempBuf;
    szFilename = szFilename + _T("\\Forensic\\AuditReport_incomplete.xml");



	INC_SYS_CALL_COUNT(MoveFile); 
	if(!MoveFile(OrigscanFile.c_str(),szFilename.c_str()))
    {
	    AppLog.Write("MoveFile Failed - AuditReport_incomplete.xml");
	    return -1;
    }

    AppLog.SetLogFile(szFilename.c_str());

    m_szRepositoryFile = wstring(g_tempBuf);
    m_szRepositoryFile = m_szRepositoryFile+ _T("\\TriageResults\\results_incomplete.xml");

	return 0;
}
#endif

//********************************************************************************
//
//  SetupCaseDirectory
//
//********************************************************************************
#ifdef PRO
tstring lattDrive;
int WSTRemote::SetupCaseDirectory()
{
    BOOL createStatus;
    tstring info;
    SysData sd;
    SYSTEMTIME st;
    _int64 availSpace;
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);

	char szCurrentDrive[4];

	
	// Get and save the current drive. This is used to identify
    // any windows that pop up from the token insertion
    INC_SYS_CALL_COUNT(GetCurrentDirectory); 
    GetCurrentDirectory(MAX_PATH, g_tempBuf);
 	char c_szText[256];
	size_t t;
	wcstombs_s(&t,c_szText,256,g_tempBuf,wcslen(g_tempBuf) + 1);
	AppLog.Write("g_tempBuf",WSLMSG_ERROR);
	AppLog.Write(c_szText,WSLMSG_ERROR);
	if(c_szText[0] != '\\' && c_szText[1] != '\\')
	{
	    //it is a drive letter 
		memset(&g_tempBuf[3],0,253);//Just set the rest to null
		lattDrive = g_tempBuf;
	}
	else
	{
		lattDrive = g_tempBuf;
		lattDrive += _T("\\");
	}	
	
	tstring caseDir = caseDrive;

#ifdef CMDLINE
	caseDir += _T("Windows\\US-LATT\\");
#endif

	caseDir += _T("CaseDirectory");


    INC_SYS_CALL_COUNT(SetCurrentDirectory); 
    if(!SetCurrentDirectory(caseDir.c_str()))
    {
        //Create a global case directory
        INC_SYS_CALL_COUNT(CreateDirectory); 
		if(!CreateDirectory(caseDir.c_str(),NULL))
        {
	        if(GetLastError() != ERROR_ALREADY_EXISTS)
	        {
		        exit(-3);
	        }
        }
        else
        {
			INC_SYS_CALL_COUNT(SetCurrentDirectory); // Needed to be INC TKH
			if(!SetCurrentDirectory(caseDir.c_str()))
            {
		        exit(-3);
            }
        }
    }

	
  
    INC_SYS_CALL_COUNT(GetSystemTime); 
    GetSystemTime(&st);
    sd.GetNetBIOSName(info);

	tstring dateTime;

	dateTime+=info;
	dateTime+=_T("_");//UserName

	TCHAR timeBuf[9];
	_tstrdate_s(timeBuf,9);
	TCHAR *loc=NULL;
	do
	{
		loc = _tstrchr(timeBuf,_T('/'));
		if(loc!=NULL)
		{
			*loc = '_';
		}
	}while(loc != NULL);

	dateTime +=timeBuf;
	dateTime +=_T("_"); //date

	memset(timeBuf,0,sizeof(TCHAR)*9);

	_tstrtime_s(timeBuf,9);

	loc=NULL;
	do
	{
		loc = _tstrchr(timeBuf,_T(':'));
		if(loc!=NULL)
		{
			*loc = '_';
		}
	}while(loc != NULL);

	dateTime +=timeBuf;//time
	

    INC_SYS_CALL_COUNT(CreateDirectory); // Needed to be INC TKH
	createStatus=CreateDirectory(dateTime.c_str(),NULL);
    if(!createStatus)
    {
	    if(GetLastError() == ERROR_ALREADY_EXISTS)
	    {
			gui.DisplayError(_T("Directory already exists. Try running the acquisition again"));
	        AppLog.Write("Directory already exists. Try running the acquisition again");
	    }
	    else
	    {
			gui.DisplayError(_T("Creating case directory"));
	        AppLog.Write("Error creating directory");
	    }
    }
	else
	{
		//Set a global casedirectory string for future use
		memset(g_tempBuf,0,sizeof(g_tempBuf));
		INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
		GetCurrentDirectory(MAX_PATH,g_tempBuf);
	}
	
	int retFromDir=0;
	
	if((retFromDir=InitializeCaseDirectory(dateTime)) != 0)//After this function completes successfully the program is in the individual case directory
	{
		gui.DisplayError(_T("In initializing case directory"));
		AppLog.Write("Error in InitializeCaseDirectory",WSLMSG_ERROR);
		
		return -1;
	}
	else
	{
			
		int retFromCopy=0;
#ifndef CMDLINE
		if(SetupLogsAndResults(m_szRepositoryFile,lattDrive+_T("Program\\AuditReport_incomplete.xml")))//Copy the AuditReport file from the Program directory of the uslatt drive
#else
		if(SetupLogsAndResults(m_szRepositoryFile,lattDrive+_T("Windows\\US-LATT\\AuditReport_incomplete.xml")))//Copy the AuditReport file from pwd directory of the casedir
#endif
		{
#ifndef CMDLINE
			//Let us Make one more try to copy from current Directory
			if(SetupLogsAndResults(m_szRepositoryFile,lattDrive+_T("AuditReport_incomplete.xml")))
								
#endif	
			{
				AppLog.Write("Error while moving AuditReport.xml",WSLMSG_ERROR);
							//It is not nice to continue.
				//exit(-1);
			}
		
		}

	
#ifndef CMDLINE //Donot copy unecessary files over in the remote.
		if((retFromCopy=CopyAuxFiles(dateTime,lattDrive))!=0)
		{
			AppLog.Write("Error while copying auxiliary files",WSLMSG_ERROR);
		}
#endif
		
	}

	 {
		INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
		GetCurrentDirectory(MAX_PATH,ca_dirName);
	}
	

    INC_SYS_CALL_COUNT(GlobalMemoryStatusEx); 
    GlobalMemoryStatusEx(&statex);

    INC_SYS_CALL_COUNT(GetDiskFreeSpaceEx); 
    GetDiskFreeSpaceEx(caseDrive,(PULARGE_INTEGER)&availSpace,NULL,NULL);
	

    if((config.getPhysicalMemory) && (availSpace < (_int64)statex.ullTotalPhys))
    {
		gui.DisplayWarning(_T("Physical Memory Dump cannot proceed due to insufficient space on the token"));
	    AppLog.Write("Complete acqusition not possible (Physical Memory not dumped due to insufficient memory on capture device)",WSLMSG_STATUS);
        config.getPhysicalMemory = false;
    }
	else if(availSpace < MEM_LIMIT)
	{
		gui.DisplayWarning(_T("Physical Memory Dump cannot proceed due to insufficient space on the token"));
		 AppLog.Write("Acqusition not possible due to insufficient memory on capture device)",WSLMSG_STATUS);
	}

    return 1;
}

#else
int WSTRemote::SetupCaseDirectory()
{
    BOOL createStatus;
    tstring info;
    SysData sd;
    SYSTEMTIME st;
    _int64 availSpace;
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    char szCurrentDrive[2];

	
	// Get and save the current drive. This is used to identify
    // any windows that pop up from the token insertion
    INC_SYS_CALL_COUNT(GetCurrentDirectory); 
    GetCurrentDirectory(MAX_PATH, g_tempBuf);
    szCurrentDrive[0] = toupper(g_tempBuf[0]);
    szCurrentDrive[1] = 0;

	tstring caseDir = g_tempBuf;
	caseDir += _T("\\AuditReport_incomplete.xml");
	

    INC_SYS_CALL_COUNT(SetCurrentDirectory); 
    if(!SetCurrentDirectory(_T("..\\CaseDirectory")))
    {
        //Create a global case directory
        INC_SYS_CALL_COUNT(CreateDirectory); 
        if(!CreateDirectory(_T("..\\CaseDirectory"),NULL))
        {
	        if(GetLastError() != ERROR_ALREADY_EXISTS)
	        {
		        exit(-3);
	        }
        }
        else
        {
			INC_SYS_CALL_COUNT(SetCurrentDirectory); // Needed to be INC TKH
            if(!SetCurrentDirectory(_T("..\\CaseDirectory")))
            {
		        exit(-3);
            }
        }
    }
  
    INC_SYS_CALL_COUNT(GetSystemTime); 
    GetSystemTime(&st);
    sd.GetNetBIOSName(info);

	tstring dateTime;

	dateTime+=info;
	dateTime+=_T("_");//UserName

	TCHAR timeBuf[9];
	_tstrdate_s(timeBuf,9);
	TCHAR *loc=NULL;
	do
	{
		loc = _tstrchr(timeBuf,_T('/'));
		if(loc!=NULL)
		{
			*loc = '_';
		}
	}while(loc != NULL);

	dateTime +=timeBuf;
	dateTime +=_T("_"); //date

	memset(timeBuf,0,sizeof(TCHAR)*9);

	_tstrtime_s(timeBuf,9);

	loc=NULL;
	do
	{
		loc = _tstrchr(timeBuf,_T(':'));
		if(loc!=NULL)
		{
			*loc = '_';
		}
	}while(loc != NULL);

	dateTime +=timeBuf;//time
	

    INC_SYS_CALL_COUNT(CreateDirectory); // Needed to be INC TKH
	createStatus=CreateDirectory(dateTime.c_str(),NULL);
    if(!createStatus)
    {
	    if(GetLastError() == ERROR_ALREADY_EXISTS)
	    {
			gui.DisplayError(_T("Directory already exists. Try running the acquisition again"));
	        AppLog.Write("Directory already exists. Try running the acquisition again");
	    }
	    else
	    {
			gui.DisplayError(_T("Creating case directory"));
	        AppLog.Write("Error creating directory");
	    }
    }
	else
	{
		//Set a global casedirectory string for future use
		memset(g_tempBuf,0,sizeof(g_tempBuf));
		INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
		GetCurrentDirectory(MAX_PATH,g_tempBuf);
	}
	
	int retFromDir=0;
	
	if((retFromDir=InitializeCaseDirectory(dateTime)) != 0)
	{
		gui.DisplayError(_T("In initializing case directory"));
		AppLog.Write("Error in InitializeCaseDirectory",WSLMSG_ERROR);
		return -1;
	}
	else
	{
		int retFromCopy=0;
		if(SetupLogsAndResults(m_szRepositoryFile,caseDir))
		{
			AppLog.Write("Error while moving AuditReport.xml",WSLMSG_ERROR);
			//It is not nice to continue.
			exit(-1);
		}
		if((retFromCopy=CopyAuxFiles(dateTime))!=0)
		{
			AppLog.Write("Error while copying auxiliary files",WSLMSG_ERROR);
			return -1;
		}
	}

    INC_SYS_CALL_COUNT(GlobalMemoryStatusEx); 
    GlobalMemoryStatusEx(&statex);

    INC_SYS_CALL_COUNT(GetDiskFreeSpaceEx); 
    GetDiskFreeSpaceEx(NULL,(PULARGE_INTEGER)&availSpace,NULL,NULL);
	
    {
		TCHAR dirName[MAX_PATH];
		INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
		GetCurrentDirectory(MAX_PATH,dirName);
		m_szCurrentCaseDir = dirName;

    }
	
    if((config.getPhysicalMemory) && (availSpace < (_int64)statex.ullTotalPhys))
    {
		gui.DisplayWarning(_T("Physical Memory Dump cannot proceed due to insufficient space on the token"));
	    AppLog.Write("Complete acqusition not possible (Physical Memory not dumped due to insufficient memory on capture device)",WSLMSG_STATUS);
        config.getPhysicalMemory = false;
    }
	else if(availSpace < MEM_LIMIT)
	{
		gui.DisplayWarning(_T("Physical Memory Dump cannot proceed due to insufficient space on the token"));
		 AppLog.Write("Acqusition not possible due to insufficient memory on capture device)",WSLMSG_STATUS);
	}

    return 1;
}
#endif

//********************************************************************************
//
//  GetLoggedInUsers
//
//********************************************************************************
int WSTRemote::GetLoggedInUsers(HashedXMLFile &repository)
{
	LPWKSTA_USER_INFO_1 userInfoBuf = NULL;
	DWORD entriesRead = 0, totalEntries = 0;
	DWORD rvApi;
    TCHAR fullname[512];

	memset(fullname,0,512*sizeof(TCHAR));
    INC_SYS_CALL_COUNT(NetWkstaUserEnum); 
	rvApi = NetWkstaUserEnum(
		NULL, 1, 
		(LPBYTE *)&userInfoBuf, 
		MAX_PREFERRED_LENGTH, 
		&entriesRead, 
		&totalEntries, 
		NULL
	);

	if ((rvApi == NERR_Success || rvApi == ERROR_MORE_DATA) && userInfoBuf != NULL) 
    {
        repository.OpenNode("LoggedInUsers");
  		for (int i=0; i < (int)entriesRead; i++) 
        {
            memset(fullname, 0, sizeof(TCHAR)*512);            
            if (userInfoBuf[i].wkui1_logon_domain)
            {
                INC_SYS_CALL_COUNT(WideCharToMultiByte); 
#ifndef _UNICODE
                WideCharToMultiByte(CP_ACP, 0, userInfoBuf[i].wkui1_logon_domain, wcslen(userInfoBuf[i].wkui1_logon_domain), (LPSTR)fullname, sizeof(fullname), 0, NULL);
#else
				_tcscpy(fullname,userInfoBuf[i].wkui1_logon_domain);
#endif
		    }
			

            memset(g_tempBuf, 0, sizeof(TCHAR)*256);  			
            INC_SYS_CALL_COUNT(WideCharToMultiByte); 
#ifndef _UNICODE
			WideCharToMultiByte(CP_ACP, 0, userInfoBuf[i].wkui1_username, wcslen(userInfoBuf[i].wkui1_username),(LPSTR)g_tempBuf, sizeof(g_tempBuf), 0, NULL);
#else
			
			printf("%d\n",_tcslen(userInfoBuf[i].wkui1_username));

			_tcscpy(g_tempBuf,userInfoBuf[i].wkui1_username);
#endif
			
            if (_tcslen(fullname) && (_tcslen(fullname) + _tcslen(g_tempBuf) < sizeof(fullname)))
            {
				_tcscat_s(fullname,_T("\\"));
				_tcscat_s(fullname,g_tempBuf);
			}
            else
            {
				_tcsncpy_s(fullname, sizeof(fullname)/sizeof(TCHAR), g_tempBuf, sizeof(fullname)/sizeof(TCHAR));
            }
			repository.WriteNodeWithValue("User", fullname);
		}
        INC_SYS_CALL_COUNT(NetApiBufferFree); 
		NetApiBufferFree(userInfoBuf);
        repository.CloseNode("LoggedInUsers");
	}
	else 
    {
		if (userInfoBuf == NULL) 
        {
        	AppLog.Write("NetWkstaUserEnum returned an error", WSLMSG_ERROR);
        }
	}
    return 0;
}


//Obtain the TimeZoneInformation
int GetTZI(TCHAR ** tziString)
{

	TCHAR *tziKey=_T("SYSTEM\\CurrentControlSet\\Control\\TimeZoneInformation");
	HKEY hkeyTZI;
	DWORD valueLen=0;
	DWORD valueNameLen=0;
	DWORD valueType=0;

	TCHAR *valueName;
	BYTE *value;

	bool bfoundStandardName,bfoundTZIKeyName;
	TCHAR *standardName=NULL;
	TCHAR *tziKeyName=NULL;

	DWORD num_tzi_subkeys,max_tzi_subkey_len,num_tzi_values,max_tzi_value_name_len,max_tzi_value_len;
	FILETIME last_write_time;

	//HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\TimeZoneInformation\TimeZoneKeyName - Vista/7
	//OR//
	//HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\TimeZoneInformation\StandardName - XP

	//1. Open the TZI key under HKLM hive

	if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,tziKey,0,KEY_READ,&hkeyTZI) != ERROR_SUCCESS) //Should be KEY_READ (KEY_ALL_ACCESS needs admin priv)
	{
		LOG_SYSTEM_ERROR(_T("RegOpenKeyEx Failed on TimeZoneInformation registry Key"));
		return ERR_REG_OPEN_KEY;
	}
	else
	{

		if(RegQueryInfoKey(hkeyTZI,
			NULL,
			NULL,
			NULL,
			&num_tzi_subkeys,
			&max_tzi_subkey_len,
			NULL,
			&num_tzi_values,
			&max_tzi_value_name_len,
			&max_tzi_value_len,
			NULL,
			&last_write_time
			) != ERROR_SUCCESS )
		{
			LOG_SYSTEM_ERROR(_T("RegQueryInfoKey Failed on TimeZoneInformation Key"));
			RegCloseKey(hkeyTZI);
			return ERR_REG_QUERY_INFO;
		}
		else
		{
			
			bfoundStandardName = false;
			bfoundTZIKeyName = false;

			valueName = new TCHAR[max_tzi_value_name_len+1];
			ZeroMemory(valueName,max_tzi_value_name_len+1);
			value = new BYTE[max_tzi_value_len+1];
			ZeroMemory(value,max_tzi_value_len+1);

			//Iterate through all the values under the TZI key
			for(int values=0;values < num_tzi_values;values++)
			{
				valueLen     = max_tzi_value_len+1;
				valueNameLen = max_tzi_value_name_len+1;

				ZeroMemory(valueName,max_tzi_value_name_len+1);
				ZeroMemory(value,max_tzi_value_len+1);

				if(RegEnumValue(hkeyTZI,
					values,
					valueName,
					&valueNameLen,
					0,
					&valueType,
					value,
					&valueLen) != ERROR_SUCCESS)
				{
					LOG_SYSTEM_ERROR(_T("Error in RegEnumValue for TimeZoneInformation"));
					RegCloseKey(hkeyTZI);
					delete [] valueName;
					valueName = NULL;
					delete [] value;
					value = NULL;
					return ERR_REG_ENUM_VALUE;
				}
				else
				{
					if(valueType == REG_SZ && (!_tcsncmp(_T("StandardName"),valueName,_tcslen(valueName))) ) //Vista and 7
					{
						bfoundStandardName = true;
						standardName = new TCHAR[max_tzi_value_len+1];
						ZeroMemory(standardName,max_tzi_value_len+1);
						_tcscpy_s(standardName,max_tzi_value_len+1,(TCHAR *) value);
#ifdef _DEBUG
						wprintf(TEXT("%s\n"),value);
#endif
					}

					if(valueType == REG_SZ && (!_tcsncmp(_T("TimeZoneKeyName"),valueName,_tcslen(valueName))) ) //Vista and 7
					{
						bfoundTZIKeyName = true;
						tziKeyName = new TCHAR[max_tzi_value_len+1];
						ZeroMemory(tziKeyName,max_tzi_value_len+1);

						_tcscpy_s(tziKeyName,max_tzi_value_len+1,(TCHAR *) value);
#ifdef _DEBUG
						wprintf(TEXT("%s\n"),value);
#endif
					}
				}
			}		
		}
	}

	//Copy and return the TZIKeyName if that is available else return StandardName
	if(bfoundTZIKeyName)
	{
		*tziString = tziKeyName;
		delete [] standardName;
		standardName=NULL;
	}
	else if(bfoundStandardName)
	{
		*tziString = standardName;
	}

	//Free resources and return
	RegCloseKey(hkeyTZI);
	delete [] valueName;
	valueName = NULL;
	delete [] value;
	value = NULL;
	return FUNC_SUCCESS;

}
//********************************************************************************
//
//  GetSystemData
//
//********************************************************************************
int WSTRemote::GetSystemData(HashedXMLFile &repository)
{
    int iRetVal = 0;
    tstring tempStr;
    tstringstream tempStream;
    tstring info;
    SysData sd;
    SysData::SysDataStatus sdStatus;
    unsigned int cpus;
    vector<CPUInfo> cpui;
    vector<UserData> users;
    vector<LongProcessData> procs;
    vector<tstring> drives;
    int nServices;
	ULARGE_INTEGER executionTime;

    AppLog.Write("WSTRemote::GetSystemData()", WSLMSG_DEBUG);
    AppLog.Write("Collecting system data.", WSLMSG_STATUS);

    // <SYSINFO>
    repository.OpenNode("SYSINFO");


    if (config.getOperatingSystem) 
    {
		gui.DisplayStatus(_T("Collecting OS information."));
	    AppLog.Write("Collecting OS Information.", WSLMSG_STATUS);
	    sdStatus = sd.GetOSVersionInformation(repository);
    }
	gui.DisplayStatus(_T("Obtaining NetBIOSName ."));
    sdStatus = sd.GetNetBIOSName(info);        	
    if (sdStatus == SysData::SUCCESS)
    {
        repository.WriteNodeWithValue("netbiosname", info.c_str());
    }
    else
    {
        repository.WriteNodeWithValue("netbiosname", "N/A");
    }


    if (config.getIPConfiguration) 
    {
		gui.DisplayStatus(_T("Collecting IP configuration information."));
	    AppLog.Write("Collecting IP Information.", WSLMSG_STATUS);
	    // DNS name
	    sdStatus = sd.GetDNSName(info);
	    if (sdStatus == SysData::SUCCESS)
        {
            repository.WriteNodeWithValue("dnsname", info.c_str());
        }
	    else
        {
            repository.WriteNodeWithValue("dnsname", "N/A");
        }
    	
	    // IP address
	    sdStatus = sd.GetIPAddress(info);
	    if (sdStatus == SysData::SUCCESS)
        {
            repository.WriteNodeWithValue("ip", info.c_str());
        }
	    else
        {
            repository.WriteNodeWithValue("ip", "N/A");
        }
    }

	gui.DisplayStatus(_T("Collecting Time Zone information."));

	//Get Time zone information
	TCHAR *tziString=NULL;
	if(GetTZI(&tziString) == FUNC_SUCCESS)
	{
		if(tziString != NULL)
		{
			 repository.WriteNodeWithValue("tzi", tziString);
#ifdef _DEBUG
			wcout<<tziString<<endl;
#endif
			delete [] tziString;
		}
	}
	else
	{
		gui.DisplayStatus(_T("DATA NOT AVAILABLE: Time Zone information."));
	}
	
	
	// Time Zone information
	//HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\TimeZoneInformation\TimeZoneKeyName - Vista/7
	////HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\TimeZoneInformation\StandardName - XP
	
	//Capture wisely XP/vista/7 TODO

    if (config.getMachineInformation) 
    {
		gui.DisplayStatus(_T("Collecting CPU information."));
	    AppLog.Write("Collecting CPU Information.", WSLMSG_STATUS);
	    sdStatus = sd.GetCPUCount(cpus);
	    if (sdStatus == SysData::SUCCESS)
        {
            _ultot_s(cpus, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
            repository.WriteNodeWithValue("cpucount", g_tempBuf);
        }
	    else
        {
            repository.WriteNodeWithValue("cpucount", "N/A");
        }


	    sdStatus = sd.GetCPUInfo(cpui);
	    if (sdStatus == SysData::SUCCESS) 
	    {
	        tempStr.assign(cpui[0].processorName.begin(), cpui[0].processorName.end());
            repository.WriteNodeWithValue("cpu", tempStr.c_str());

	        for (vector<CPUInfo>::iterator iter = cpui.begin(); iter != cpui.end(); ++iter) 
            {
                repository.OpenNodeWithAttributes("extendedcpu");
                repository.WriteAttribute("vendor", iter->vendorName.c_str());
                repository.WriteAttribute("name", iter->processorName.c_str());

                repository.WriteAttribute("speed", iter->szSpeed.c_str());
                _itot_s(iter->iFamily, g_tempBuf,  sizeof(g_tempBuf)/sizeof(TCHAR), 10);
                repository.WriteAttribute("family", g_tempBuf);
                _itot_s(iter->iModel, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
                repository.WriteAttribute("model", g_tempBuf);
                _itot_s(iter->iStepping, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
                repository.WriteAttribute("stepping", g_tempBuf);
		        tempStr.clear();
		        if (iter->dwFeatures & SUPPORT_MMX)
			        tempStr += TEXT("MMX;");
		        if (iter->dwFeatures & SUPPORT_SSE)
			        tempStr += TEXT("SSE;");
		        if (iter->dwFeatures & SUPPORT_SSE2)
			        tempStr += TEXT("SSE2;");
		        if (iter->dwFeatures & SUPPORT_3DNOW)
			        tempStr += TEXT("AMD_3DNOW;");
				if (iter->dwFeatures & SUPPORT_LM)
			        tempStr += TEXT("LONG_MODE_SUPPORTED");
                repository.WriteAttribute("features", tempStr.c_str());
                repository.Write("/>");
	        }
	    }
	    else
        {
            repository.WriteNodeWithValue("cpu", "N/A");
        }
    }

    if (config.getAllUsers)
    {
		gui.DisplayStatus(_T("Collecting User Information."));
	    AppLog.Write("Collecting User Information.", WSLMSG_STATUS);
	    sdStatus = sd.GetLocalUsers(users);
	    if (sdStatus == SysData::SUCCESS) 
	    {
            repository.OpenNode("users");
	        for (vector<UserData>::const_iterator iter = users.begin(); iter != users.end(); iter++) 
	        {
		        tempStr.assign(iter->name.begin(), iter->name.end());
                repository.Write(tempStr.c_str());
                repository.Write(";");
	        }
            repository.CloseNode("users");

            repository.OpenNode("extendedusers");
	        // UserData is all in wide strings so we're using a temp tstring in case conversion is necessary
	        //  (handled by string class)
	        for (vector<UserData>::iterator i = users.begin(); i != users.end(); ++i) 
	        {	
                repository.OpenNodeWithAttributes("user");
		        tempStr.assign(i->name.begin(), i->name.end());
                repository.WriteAttribute("name", tempStr.c_str());

		        tempStr.assign(i->comment.begin(), i->comment.end());
                repository.WriteAttribute("comment", tempStr.c_str());

		        tempStr.assign(i->usr_comment.begin(), i->usr_comment.end());
                repository.WriteAttribute("user_comment", tempStr.c_str());

		        tempStr.assign(i->full_name.begin(), i->full_name.end());
                repository.WriteAttribute("full_name", tempStr.c_str());

                tempStr.clear();
		        switch(i->priv) {
			        case USER_PRIV_GUEST: 
                        tempStr = TEXT("guest"); 
				        break;
			        case USER_PRIV_USER: 
                        tempStr = TEXT("user"); 
				        break;
			        case USER_PRIV_ADMIN: 
                        tempStr = TEXT("admin"); 
				        break;
		        }
                if (tempStr.length())
                {
                    repository.WriteAttribute("privilege", tempStr.c_str());
                }

		        tempStr.clear();
		        if (i->auth_flags & AF_OP_PRINT) 
			        tempStr += TEXT("print ");
		        if (i->auth_flags & AF_OP_ACCOUNTS)
			        tempStr += TEXT("accounts ");
		        if (i->auth_flags & AF_OP_COMM)
			        tempStr += TEXT("comm ");
		        if (i->auth_flags & AF_OP_SERVER)
			        tempStr += TEXT("server ");
                if (tempStr.length())
                {
                    repository.WriteAttribute("auth_flags", tempStr.c_str());
                }

                _ultot_s(i->password_age, g_tempBuf, 10);
                repository.WriteAttribute("password_age", g_tempBuf);
		        tempStr.assign(i->home_dir.begin(), i->home_dir.end());
                repository.WriteAttribute("home_dir", tempStr.c_str());

                repository.WriteAttribute("params", "");

                
				string elapsedTime;
				elapsedTime.clear();
				SecondsSince1970(i->last_logon,elapsedTime);
				repository.WriteAttribute("last_logon ", elapsedTime.c_str());

                
				elapsedTime.clear();
				SecondsSince1970(i->last_logoff,elapsedTime);
				repository.WriteAttribute("last_logoff ", elapsedTime.c_str());

                _ultot_s(i->bad_pw_count, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
                repository.WriteAttribute("bad_pw_count", g_tempBuf);

                _ultot_s(i->num_logons, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
                repository.WriteAttribute("num_logons", g_tempBuf);

		        tempStr.assign(i->logon_server.begin(), i->logon_server.end());
                repository.WriteAttribute("logon_server", tempStr.c_str());

                _ultot_s(i->country_code, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
                repository.WriteAttribute("country_code", g_tempBuf);

		        tempStr.assign(i->workstations.begin(), i->workstations.end());
                repository.WriteAttribute("workstations", tempStr.c_str());

                _ultot_s(i->max_storage, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
                repository.WriteAttribute("max_storage", g_tempBuf);

                _ultot_s(i->code_page, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
                repository.WriteAttribute("code_page", g_tempBuf);

                repository.Write("/>");

	        }
            repository.CloseNode("extendedusers");
	    } 
	    else 
	    {
            repository.WriteNodeWithValue("users", "N/A");
	    }

		
	    if(GetLoggedInUsers(repository))//Fix this function at the moment it does not return errors //FIXME
	    {
			;
	    }
	
    }

    if (config.getStorageDevices) {
		gui.DisplayStatus(_T("Collecting Storage Devices information."));
        // Always get the drive info for Gargoyle now that DiskInfo.xml handles LiveWire
        AppLog.Write("Collecting Storage Information.", WSLMSG_STATUS);
        repository.OpenNode("fixeddrives");
        sdStatus = sd.GetDrives(DRIVE_FIXED, false, drives);
        if (sdStatus == SysData::SUCCESS) 
        {
	        for(vector<tstring>::iterator i = drives.begin(); i != drives.end(); i++) 
            {
		        if (i != drives.begin()) 
                {
                    repository.Write(";");
		        }
                repository.Write(i->c_str());
	        }
        }
        else 
        {
            repository.Write("N/A");
        }
        repository.CloseNode("fixeddrives");

        repository.OpenNode("removabledrives");
        drives.clear();
        sdStatus = sd.GetDrives(DRIVE_REMOVABLE, true, drives);
        if (sdStatus == SysData::SUCCESS) 
        {
	        for(vector<tstring>::iterator i = drives.begin(); i != drives.end(); i++) 
            {
		        if (i != drives.begin()) 
                {
                    repository.Write(";");
		        }
                repository.Write(i->c_str());
	        }
        }
        else 
        {
            repository.Write("N/A");
        }
        repository.CloseNode("removabledrives");
    }

    if(config.getDrivers)
    {
		gui.DisplayStatus(_T("Collecting Driver Information."));
		sd.GetDriverInfo(repository);
    }

    if (config.getGetProcesses) 
    {
		gui.DisplayStatus(_T("Collecting Process Information."));
	    AppLog.Write("Collecting Process Information.", WSLMSG_STATUS);
        repository.OpenNode("processes");

	    if (sd.GetLongProcessData(procs) == SysData::SUCCESS) {

		    for(vector<LongProcessData>::const_iterator i = procs.begin(); i != procs.end(); ++i) {
                repository.OpenNodeWithAttributes("process");
                _ultot_s(i->pid, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
                repository.WriteAttribute("pid", g_tempBuf);
                repository.WriteAttribute("name", i->exeName.c_str());
                _ultot_s(i->numThreads, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
                repository.WriteAttribute("numthreads", g_tempBuf);
                _ultot_s(i->ppid, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
			    repository.WriteAttribute("parentpid", g_tempBuf);
			    repository.WriteAttribute("path", i->path.c_str());
                _ltot_s(i->priority, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
			    repository.WriteAttribute("priority", g_tempBuf);
			    repository.WriteAttribute("user", i->userAccount.c_str());
			    repository.WriteAttribute("usertype", i->userType.c_str());
			    repository.WriteAttribute("userdomain", i->userDomain.c_str());

			    executionTime.LowPart = i->userTime.dwLowDateTime;
			    executionTime.HighPart = i->userTime.dwHighDateTime;
			    if (_ui64tot_s(executionTime.QuadPart, g_tempBuf, 256, 10) == 0) {
				    repository.WriteAttribute("usertime", g_tempBuf);
			    }
			    else {
//				    cout << "craP";
			    }
			    executionTime.LowPart = i->kernelTime.dwLowDateTime;
			    executionTime.HighPart = i->kernelTime.dwHighDateTime;
			    _ui64tot_s(executionTime.QuadPart, g_tempBuf, 256, 10);
			    repository.WriteAttribute("kerneltime", g_tempBuf);

			    // Format the time correctly...
			    _stprintf_s(g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), TEXT("%02d/%02d/%02d %d:%02d:%02d.%03d"), i->startTime.wMonth, i->startTime.wDay, i->startTime.wYear, i->startTime.wHour, i->startTime.wMinute, i->startTime.wSecond, i->startTime.wMilliseconds);
			    tempStr.clear();
			    tempStr += g_tempBuf;

			    repository.WriteAttribute("starttime", tempStr.c_str()); //SYSTEMTIME
                repository.Write("/>");  // process
		    }
	    }
	    else 
        {
            repository.Write("N/A");
	    }
        repository.CloseNode("processes");

    }

    if (config.getServiceStatus) 
    {
		gui.DisplayStatus(_T("Collecting Services Information."));
	    AppLog.Write("Collecting Service and Driver Information.", WSLMSG_STATUS);
	    EnumerateServices(repository, nServices);
    }

    repository.CloseNode("SYSINFO");
    AppLog.Write("Done collecting system data.", WSLMSG_STATUS);

	
    return iRetVal;
}

//********************************************************************************
//
//  EnumerateServices - collect services data
//
//********************************************************************************
int WSTRemote::EnumerateServices(HashedXMLFile &repository, int &iServiceCount)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService   = NULL;
    DWORD nBytesNeeded	  = 0;
    DWORD nBytesRemaining = 0;
    DWORD nEnumSize		  = 0;
    DWORD nNumServices	  = 0;
    LPENUM_SERVICE_STATUS_PROCESS lpEnum   = NULL;
    LPQUERY_SERVICE_CONFIG lpServiceConfig = NULL;
    LPSERVICE_DESCRIPTION lpServiceDesc = NULL;
//    XMLNode root, child, gchild;
    TCHAR tempBuf[25];
    tstring szTemp;
    SERVICE_STATUS serviceStatus;

	
    // Open the Service Control Manager
    INC_SYS_CALL_COUNT(OpenSCManager); 
    hSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, GENERIC_READ);
    if(!hSCManager)
	    return 0;

    // Query to see how much room we need for lpEnum
    INC_SYS_CALL_COUNT(EnumServicesStatusEx); 
    EnumServicesStatusEx(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_DRIVER | SERVICE_WIN32, SERVICE_STATE_ALL, NULL, 0, &nBytesNeeded, &nNumServices, 0, NULL);
    nEnumSize = nBytesNeeded;

    // Allocation lpEnum
    INC_SYS_CALL_COUNT(GetProcessHeap); 
    INC_SYS_CALL_COUNT(HeapAlloc); 
    lpEnum = (LPENUM_SERVICE_STATUS_PROCESS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nEnumSize);

    nBytesNeeded = 0;

    // Query all of the services
    INC_SYS_CALL_COUNT(EnumServicesStatusEx); 
    if (!EnumServicesStatusEx(hSCManager, SC_ENUM_PROCESS_INFO, SERVICE_DRIVER | SERVICE_WIN32, SERVICE_STATE_ALL, (LPBYTE)lpEnum, nEnumSize, &nBytesNeeded, &nNumServices, 0, NULL))
    {
        INC_SYS_CALL_COUNT(GetProcessHeap); 
        INC_SYS_CALL_COUNT(HeapFree); 
        HeapFree(GetProcessHeap(), 0, lpEnum);
    	AppLog.Write("Service enumeration failed", WSLMSG_ERROR);
	    return 0;
    }

    repository.OpenNode("Services");
    // nNumServices is populated on the SECOND call that actually enumerates the services.
    iServiceCount = nNumServices; 
    for(DWORD i = 0; i < nNumServices; i++)
    {
	    // Open each service to extract data
        INC_SYS_CALL_COUNT(OpenService); 
	    hService = OpenService(hSCManager, lpEnum[i].lpServiceName, GENERIC_READ);

	    // Validate the handle
	    if(hService)
	    {
		    nBytesNeeded = 0;
		    nBytesRemaining = 0;
            INC_SYS_CALL_COUNT(QueryServiceConfig); 
		    QueryServiceConfig(hService, NULL, 0, &nBytesNeeded);

            INC_SYS_CALL_COUNT(GetProcessHeap); 
            INC_SYS_CALL_COUNT(HeapAlloc); 
			
		    lpServiceConfig = (LPQUERY_SERVICE_CONFIG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nBytesNeeded);

		    // Validate lpServiceConfig

		    if(lpServiceConfig)
		    {
                repository.OpenNodeWithAttributes("service");
                repository.WriteAttribute("serviceName", lpEnum[i].lpServiceName);
                INC_SYS_CALL_COUNT(QueryServiceConfig); 
			    if (!QueryServiceConfig(hService, lpServiceConfig, nBytesNeeded, &nBytesRemaining))
                {
#ifdef _DEBUG
                    DWORD dwError;
                    char szBuf[64];
                    
                    dwError = GetLastError();
                    sprintf_s(szBuf, "QueryServiceConfig failed: %d", dwError);
                     AppLog.Write(szBuf, WSLMSG_DEBUG);
#endif
                }
                else
                {

			        // service type
			        if (lpServiceConfig->dwServiceType & SERVICE_FILE_SYSTEM_DRIVER)
				        szTemp = TEXT("File System Driver");
			        if (lpServiceConfig->dwServiceType & SERVICE_KERNEL_DRIVER)
				        szTemp = TEXT("Kernel Driver");
			        if (lpServiceConfig->dwServiceType & SERVICE_WIN32_OWN_PROCESS)
				        szTemp = TEXT("Win32 Process (owned)");
			        if (lpServiceConfig->dwServiceType & SERVICE_WIN32_SHARE_PROCESS)
				        szTemp = TEXT("Win32 Process (shared)");
			        if (lpServiceConfig->dwServiceType & SERVICE_INTERACTIVE_PROCESS)
				        szTemp = TEXT("Interactive ") + szTemp;			
                    repository.WriteAttribute("type", szTemp.c_str()); 

			        // service state
			        switch(lpEnum[i].ServiceStatusProcess.dwCurrentState) {
				        case SERVICE_CONTINUE_PENDING: 
                            szTemp = _T("Continuing"); 
                            break;
				        case SERVICE_PAUSE_PENDING: 
                            szTemp = _T("Pausing"); 
                            break;
				        case SERVICE_PAUSED: 
                            szTemp = _T("Paused"); 
                            break;
				        case SERVICE_RUNNING: 
                            szTemp = _T("Running"); 
                            break;
				        case SERVICE_START_PENDING: 
                            szTemp = _T("Starting"); 
                            break;
				        case SERVICE_STOP_PENDING: 
                            szTemp = _T("Stopping"); 
                            break;
				        case SERVICE_STOPPED: 
                            szTemp = _T("Stopped"); 
                            break;
				        default:
                            szTemp = _T("Unknown"); 
				            break;
			        }
                    repository.WriteAttribute("status", szTemp.c_str()); 
    			
                    if (_tcslen(lpServiceConfig->lpDependencies))
                    {
                       repository.WriteAttribute("dependencies", lpServiceConfig->lpDependencies);
                    }

                    if (_tcslen(lpServiceConfig->lpServiceStartName))
                    {
                       repository.WriteAttribute("serviceStartName", lpServiceConfig->lpServiceStartName);
                    }

                    if (_tcslen(lpServiceConfig->lpDisplayName))
                    {
                       repository.WriteAttribute("displayName", lpServiceConfig->lpDisplayName);
                    }

                    if (_tcslen(lpServiceConfig->lpBinaryPathName))
                    {
                        tstring szValue;
                        szValue = lpServiceConfig->lpBinaryPathName;

                        //szValue = XMLDelimitString(szValue);
                        repository.WriteAttribute("binaryPathName", szValue.c_str());
                    }

			        // startup flag
                    switch (lpServiceConfig->dwStartType) 
                    {
                    case SERVICE_BOOT_START:
    			        szTemp = TEXT("BOOT");
                        break;
                    case SERVICE_SYSTEM_START:
    			        szTemp = TEXT("SYSTEM");
                        break;
                    case SERVICE_AUTO_START:
    			        szTemp = TEXT("AUTO");
                        break;
                    case SERVICE_DEMAND_START:
    			        szTemp = TEXT("DEMAND");
                        break;
                    case SERVICE_DISABLED:
    			        szTemp = TEXT("DISABLED");
                        break;
			        }
                    if (szTemp.length())
                    {
                       repository.WriteAttribute("startuptype", szTemp.c_str());
                    }

					

                    if ((lpEnum[i].ServiceStatusProcess).dwServiceFlags)
                    {
			            _ultot_s(lpEnum[i].ServiceStatusProcess.dwServiceFlags, tempBuf, 10);
                        repository.WriteAttribute("tagID", szTemp.c_str());
                    }
                    if (_tcslen(lpServiceConfig->lpLoadOrderGroup))
                    {
                        repository.WriteAttribute("loadOrderGroup", szTemp.c_str());
                    }
                }
		    }

            INC_SYS_CALL_COUNT(QueryServiceStatus); 
            if (QueryServiceStatus(hService, &serviceStatus))
            {
                szTemp = _T("");
                if (serviceStatus.dwControlsAccepted & SERVICE_ACCEPT_NETBINDCHANGE)
                {
                    if (szTemp.length() == 0)
                    {
                        szTemp = _T("(ACCEPT_NETBINDCHANGE");
                    }
                    else
                    {
                        szTemp = szTemp + _T(", ACCEPT_NETBINDCHANGE");
                    }
                }
                if (serviceStatus.dwControlsAccepted & SERVICE_ACCEPT_STOP)
                {
                    if (szTemp.length() == 0)
                    {
                        szTemp = _T("(ACCEPT_STOP");
                    }
                    else
                    {
                        szTemp = szTemp + _T(", ACCEPT_STOP");
                    }
                }
                if (serviceStatus.dwControlsAccepted & SERVICE_ACCEPT_PAUSE_CONTINUE)
                {
                    if (szTemp.length() == 0)
                    {
                        szTemp = _T("(ACCEPT_PAUSE_CONTINUE");
                    }
                    else
                    {
                        szTemp = szTemp +_T(", ACCEPT_PAUSE_CONTINUE");
                    }
                }
                if (serviceStatus.dwControlsAccepted & SERVICE_ACCEPT_SHUTDOWN)
                {
                    if (szTemp.length() == 0)
                    {
                        szTemp =_T("(ACCEPT_SHUTDOWN");
                    }
                    else
                    {
                        szTemp = szTemp +_T(", ACCEPT_SHUTDOWN");
                    }
                }
                if (serviceStatus.dwControlsAccepted & SERVICE_ACCEPT_PARAMCHANGE)
                {
                    if (szTemp.length() == 0)
                    {
                        szTemp =_T("(ACCEPT_PARAMCHANGE");
                    }
                    else
                    {
                        szTemp = szTemp +_T(", ACCEPT_PARAMCHANGE");
                    }
                }
                if (serviceStatus.dwControlsAccepted & SERVICE_ACCEPT_HARDWAREPROFILECHANGE)
                {
                    if (szTemp.length() == 0)
                    {
                        szTemp =_T("(ACCEPT_HARDWAREPROFILECHANGE");
                    }
                    else
                    {
                        szTemp = szTemp +_T(", CCEPT_HARDWAREPROFILECHANGE");
                    }
                }
                if (serviceStatus.dwControlsAccepted & SERVICE_ACCEPT_POWEREVENT)
                {
                    if (szTemp.length() == 0)
                    {
                        szTemp =_T("(ACCEPT_POWEREVENT");
                    }
                    else
                    {
                        szTemp = szTemp +_T(", ACCEPT_POWEREVENT");
                    }
                }
                if (serviceStatus.dwControlsAccepted & SERVICE_ACCEPT_SESSIONCHANGE)
                {
                    if (szTemp.length() == 0)
                    {
                        szTemp =_T("(ACCEPT_SESSIONCHANGE");
                    }
                    else
                    {
                        szTemp = szTemp +_T(", ACCEPT_SESSIONCHANGE");
                    }
                }
                if (serviceStatus.dwControlsAccepted & SERVICE_ACCEPT_PRESHUTDOWN)
                {
                    if (szTemp.length() == 0)
                    {
                        szTemp =_T("(ACCEPT_STOP");
                    }
                    else
                    {
                        szTemp = szTemp +_T(", ACCEPT_STOP");
                    }
                }
                if (szTemp.length() > 0)
                {
                    szTemp = szTemp + _T(")");
                    repository.WriteAttribute("controls", szTemp.c_str());
                }
            }

            INC_SYS_CALL_COUNT(QueryServiceConfig2); 
			
		    if(QueryServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, NULL, 0, &nBytesNeeded))
			{
				/*
#ifdef		_DEBUG
				printf("GetLastError %d\n",GetLastError());
#endif
				*/
				AppLog.Write("QueryServiceConfig2 Failed",WSLMSG_ERROR);
			}
            INC_SYS_CALL_COUNT(HeapAlloc); 
			INC_SYS_CALL_COUNT(GetProcessHeap); // Needed to be INC TKH
		    lpServiceDesc = (LPSERVICE_DESCRIPTION) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nBytesNeeded);
		    if (lpServiceDesc != NULL) 
            {
			    // make sure to check that there is actually a description!
                INC_SYS_CALL_COUNT(QueryServiceConfig2); 
			    if (QueryServiceConfig2(hService, SERVICE_CONFIG_DESCRIPTION, (LPBYTE) lpServiceDesc, 
				    nBytesNeeded, &nBytesRemaining) && lpServiceDesc->lpDescription != NULL) 
                {
                    if (_tcslen(lpServiceDesc->lpDescription))
                    {
                        //repository.WriteNodeWithValue("Description", lpServiceDesc->lpDescription);
						//repository.WriteAttribute("description", lpServiceDesc->lpDescription); --Put it back in after resolving the UTF-8 issue. with IntelA-circumflexProset
                    }
			    }
                INC_SYS_CALL_COUNT(GetProcessHeap); 
                INC_SYS_CALL_COUNT(HeapFree); 
    			HeapFree(GetProcessHeap(), 0, lpServiceDesc);
		    }
			else
			{

#ifdef		_DEBUG
				printf("GetLastError %d\n",GetLastError());

#endif
				AppLog.Write("HeapAlloc Failed while allocating memory for GetServiceInfo",WSLMSG_ERROR);
			}
            // end of service attributes
            repository.Write("/>");   //Ticket #5
            //repository.CloseNode("service"); //Ticket #5

            INC_SYS_CALL_COUNT(HeapFree); 
            INC_SYS_CALL_COUNT(GetProcessHeap); 
		    HeapFree(GetProcessHeap(), 0, lpServiceConfig);
		    lpServiceConfig = NULL;			

            INC_SYS_CALL_COUNT(CloseServiceHandle); 
		    CloseServiceHandle(hService);
		    hService = NULL;
	    }
	    else
	    {
		    // Skip it for now...
	    }
    }

    repository.CloseNode("Services");

    INC_SYS_CALL_COUNT(HeapFree); 
    INC_SYS_CALL_COUNT(GetProcessHeap); 
    HeapFree(GetProcessHeap(), 0, lpEnum);
    INC_SYS_CALL_COUNT(CloseServiceHandle); 
    CloseServiceHandle(hSCManager);

    return 1;
}

//Can you believe this bug !!!
	// refer http://blogs.msdn.com/b/oldnewthing/archive/2009/06/01/9673254.aspx -- RJM


int WriteHashes(tstring filename=_T("NULL"),tstring hashValue=_T("NULL"))
{
	TCHAR g_tempBuf[MAX_PATH];
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH,g_tempBuf);
	FILE *hashFile=NULL;
	
	tstring digest_name=g_tempBuf;
	digest_name += _T("\\Forensic\\digest.xml");

	if((filename == _T("NULL")) && (hashValue== _T("NULL")))
	{
		_tfopen_s(&hashFile,digest_name.c_str(), _T("a"));
		_ftprintf_s(hashFile, _T("</Hashes>\r\n"));
		fclose(hashFile);
		return 1;
	}

	if(_tfopen_s(&hashFile,digest_name.c_str(), _T("r")) !=0 )//File does not exist create it
	{
		_tfopen_s(&hashFile,digest_name.c_str(), _T("w")); 
		_ftprintf_s(hashFile, _T("%s"), _T("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"));
		_ftprintf_s(hashFile,_T("%s"), _T("<?xml-stylesheet type=\"text/xsl\" href=\"digests.xslt\"?>\n"));
		_ftprintf_s(hashFile, _T("<Hashes>\r\n"));
		_ftprintf_s(hashFile, _T("    <MD5 Filename=\"%s\">"),filename.c_str());
		_ftprintf_s(hashFile,_T("%s</MD5>\n"),hashValue.c_str());
		fclose(hashFile);
		return 0;
	}
	else
	{
		fclose(hashFile);
		_tfopen_s(&hashFile,digest_name.c_str(), _T("a+")); 
		_ftprintf_s(hashFile, _T("    <MD5 Filename=\"%s\">"),filename.c_str());
		_ftprintf_s(hashFile,_T("%s</MD5>\n"),hashValue.c_str());
		fclose(hashFile);
		return 0;
	}
}

int insertHash(const TCHAR * filename)
{
	FILE *fd;
	tstring hash;
	tstring m_szRepositoryFile;
	unsigned char m_repositoryHash[20];
	FileHasher m_hasher;

	memset(g_tempBuf,0,MAX_PATH);
	memset(m_repositoryHash,0,sizeof(m_repositoryHash));
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH,g_tempBuf);
	if(_tcscmp(filename,_T("\\Forensic\\AuditReport.xml")))
	{
		AppLog.Write("In insertHash",WSLMSG_STATUS);
		AppLog.Write(filename,WSLMSG_STATUS);
	}
	m_szRepositoryFile=tstring(g_tempBuf)+ filename;
	if(_tfopen_s(&fd,m_szRepositoryFile.c_str(),_T("r")) == 0)
	{
		fclose(fd);
		memset(m_repositoryHash,0,sizeof(m_repositoryHash));
		hash.clear();

		m_hasher.MD5HashFile(m_szRepositoryFile.c_str(), m_repositoryHash);
		hash=m_hasher.MD5BytesToString(m_repositoryHash);
		WriteHashes(filename,hash);
	}
	else
	{
		if(_tcscmp(filename,_T("\\Forensic\\AuditReport.xml")))
		{
			AppLog.Write("Could not hash the file",WSLMSG_ERROR);
		}
		return -1;
	}
	return 0;
}

int XMLPostProcess(const TCHAR *first,const TCHAR *second,const TCHAR *third)
{
		FILE *fd=NULL;

		if((first != _T("\\Forensic\\AuditReport_incomplete.xml")))
		{
			AppLog.Write("In XMLPostProcess",WSLMSG_STATUS);
			AppLog.Write(third,WSLMSG_STATUS);
		}

		memset(g_tempBuf,0,MAX_PATH);
		INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
		GetCurrentDirectory(MAX_PATH,g_tempBuf);
		
		tstring srcFile = g_tempBuf;
		tstring destFile = g_tempBuf;

		if(first != NULL)
		{
			srcFile += first;
		}


		if(!(_tfopen_s(&fd,(srcFile).c_str(),_T("r"))))
		{
			//First Purify the file
			fclose(fd);//Close it 

			
			if((second != NULL)) // Get in only if all the arguments are not null
			{

				//destFile += _T("\\Files\\Collected\\collected_files_purify.xml");
				destFile += second;


				//First purify the file the file
				if((first != _T("\\Forensic\\AuditReport_incomplete.xml")))
				{
					AppLog.Write("Purifying XML file",WSLMSG_STATUS);
				}
				int retPurify= utfchecker(to_utf8(srcFile),to_utf8(destFile));


				if(!retPurify)
				{

					srcFile = g_tempBuf;
					//				srcFile += _T("\\Files\\Collected\\collected_files_incomplete.xml");
					srcFile += second;
					destFile = g_tempBuf;
					//				destFile += _T("\\Files\\Collected\\collected_files.xml");
					destFile += third;
					INC_SYS_CALL_COUNT(MoveFile); // Needed to be INC TKH
					if(!MoveFileEx((srcFile).c_str(),(destFile).c_str(),MOVEFILE_REPLACE_EXISTING))
					{
						//Second move the file
						if((first != _T("\\Forensic\\AuditReport_incomplete.xml")))
						{
							AppLog.Write("MoveFile Failed:(Try manual rename)");
						}
						return -1;
					}
					else
					{
						//Third hash the file and write to digest
						if(!(insertHash(third)))
						{
							return -1;
						}
					}
				}
				else
				{
					if((first != _T("\\Forensic\\AuditReport_incomplete.xml")))
					{
						AppLog.Write("Purifying XML file collected_files_incomplete.xml failed",WSLMSG_STATUS);
					}

				}
			}
			else//Get in if all you need to do is move and hash
			{
					if(first != NULL)
					{
						srcFile = g_tempBuf;
						srcFile += first;
					}
					destFile = g_tempBuf;
					destFile += third;
					INC_SYS_CALL_COUNT(MoveFile); // Needed to be INC TKH
					if((!MoveFileEx((srcFile).c_str(),(destFile).c_str(),MOVEFILE_REPLACE_EXISTING)) && (first != NULL)) //Get in for move and hash
					{
						//Second move the file
						if((first != _T("\\Forensic\\AuditReport_incomplete.xml")))
						{
							AppLog.Write("MoveFile Failed:(Try manual rename)");
						}
						return -1;
					}
					else//Get in for only hash
					{
						//Third hash the file and write to digest
						if(!(insertHash(third)))
						{
							return -1;
						}
					}

			}

		}
		else if(first == NULL)//Just Hash
		{
			if(!(insertHash(third)))
			{
				return 0;
			}
			else
			{
				return -1;
			}
		}
		return 0;
}

int WSTRemote::MoveAndHashResults()
{
		
		XMLPostProcess(NULL,NULL,_T("\\Setup\\ScanConfig.xml"));

		XMLPostProcess(_T("\\TriageResults\\results_incomplete.xml"),NULL,_T("\\TriageResults\\results.xml"));

		XMLPostProcess(_T("\\Files\\MRU\\mrudigest_incomplete.xml"),_T("\\Files\\MRU\\mrudigest_purify.xml"),
			_T("\\Files\\MRU\\mrudigest.xml"));

#ifdef RECENTDOCS
		XMLPostProcess(_T("\\Files\\MRD\\recentdocs_incomplete.xml"),_T("\\Files\\MRD\\recentdocs_purify.xml"),
			_T("\\Files\\MRD\\recentdocs.xml"));
#endif

		XMLPostProcess(_T("\\Files\\Collected\\collected_files_incomplete.xml"),_T("\\Files\\Collected\\collected_files_purify.xml"),
			_T("\\Files\\Collected\\collected_files.xml"));

		XMLPostProcess(_T("\\Screenshots\\screenshot_digest_incomplete.xml"),_T("\\Screenshots\\screenshot_digest_purify.xml"),
			_T("\\Screenshots\\screenshot_digest.xml"));

#ifdef MAILFILECAPTURE
		XMLPostProcess(_T("\\Files\\MailFiles\\MailFiles_incomplete.xml"),_T("\\Files\\MailFiles\\MailFiles_purify.xml"),
			_T("\\Files\\MailFiles\\MailFiles.xml"));
#endif
#ifdef WEBFORENSICS
		XMLPostProcess(_T("\\Files\\Web\\WebForensics_incomplete.xml"),_T("\\Files\\Web\\WebForensics_purify.xml"),
			_T("\\Files\\Web\\WebForensics.xml"));
#endif
#ifdef TIMELINE
		XMLPostProcess(_T("\\Files\\Timeline\\EventRecords_incomplete.xml"),_T("\\Files\\TimeLine\\EventRecords_purify.xml"),
			_T("\\Files\\TimeLine\\EventRecords.xml"));
#endif
		return 0;
}


int CleanUpTempFiles(const TCHAR *pathname,const TCHAR *filename)
{
	AppLog.Write("In CleanUpTempFiles",WSLMSG_STATUS);
	ifstream fd;
	fd.open(filename);
	if(!fd.fail())
	{
		fd.close();
		memset(g_tempBuf,0,MAX_PATH);
		_tcscat_s(g_tempBuf,MAX_PATH,_T("Deleting file "));
		_tcscat_s(g_tempBuf,MAX_PATH,_T("\\"));
		_tcscat_s(g_tempBuf,MAX_PATH,filename);
		AppLog.Write(g_tempBuf,WSLMSG_STATUS);
		_tunlink(filename);//We don't need this either.
	}
	else
	{
		_tcscat_s(g_tempBuf,MAX_PATH,_T("Could not delete file "));
		_tcscat_s(g_tempBuf,MAX_PATH,_T("\\"));
		_tcscat_s(g_tempBuf,MAX_PATH,filename);
		AppLog.Write(g_tempBuf,WSLMSG_STATUS);
		return -1;
	}
	return 0;
}


/*
// Initialize common structure for use by modules 
void initCommonResources(WSTCommon &commonResources, gui_resources &gui, WSTLog &AppLog, HashedXMLFile &repository, OSVERSIONINFOEX &osvex, ScanConfig &config)
{
	//memset(&commonResources, 0, sizeof(WSTCommon));
	commonResources.gui = gui;
	commonResources.AppLog = AppLog;
	commonResources.repository = repository;
	commonResources.osvex = osvex;
	commonResources.config = config;
}
*/

bool cleanupDuringAbort=false;

char serialNumber[2*(MAX_PATH) + 1];//extern in usb_serial_validate.cpp to record the serial number
int WSTRemote::DoInvestigation() 
{
	AppLog.Write("WSTRemote::DoInvestigation()", WSLMSG_DEBUG);
	int Timeout = 0;
    HashedXMLFile repository;
	WSTCommon commonResources;

	AppLog.Write("Starting Investigation.", WSLMSG_STATUS);



    // do this first!!
	if (config.getPhysicalMemory)
    {
		cleanupDuringAbort = true;
		gui.DisplayState(_T("Physical Memory ..."));
		gui.DisplayStatus(_T("Performing physical memory capture ..."));
		gui.DisplayStatus(_T("May take some time please wait  ..."));
		
		//gui.DisplayResult(_T("Physical Memory Capture: In Progress"));
		ClearSysCallsLog();
		CapturePhysicalMemory();
        ReportSysCalls("Physical Memory Capture");
		gui.ResetProgressBar();
    }

    // open the xml evidence file and get things started
	if (repository.Open(const_cast<char *> (to_utf8(m_szRepositoryFile).c_str())))
    {
		//gui.DisplayError(const_cast<wchar_t *>(m_szRepositoryFile.c_str()));
		gui.DisplayError(_T("Fatal Error"));
		gui.DisplayResult(_T("Aborting Triage"));
		gui.DisplayStatus(_T("Cannot perform File I/O"));
        // basic file I/O is not possible, just exit
		return ERR_REPOSITORY_OPEN;
    }

    // add the style sheet reference
    repository.Write("<?xml-stylesheet type=\"text/xsl\" href=\"results.xslt\"?>\n");

#ifdef STRIKE
	repository.OpenNode("STRIKELIVE_Repository");
#else
	repository.OpenNode("USLATT_Repository");
#endif
	
	//Record Product information
	repository.OpenNode("ProductInformation");
	repository.Write("\n");
#ifdef STRIKE
	repository.Write("\t");
	repository.WriteNodeWithValue("Product","STRIKE Live");
#else
	repository.Write("\t");
	repository.WriteNodeWithValue("Product","US-LATT PRO");
#endif
	repository.Write("\t");
	repository.WriteNodeWithValue("SoftwareVersion","2.1.0");
	repository.Write("\t");
	repository.WriteNodeWithValue("TokenSerialNumber",serialNumber);
	repository.CloseNode("ProductInformation");

	if (config.getSysData) 
    {
		gui.DisplayState(_T("System Data"));
		gui.DisplayStatus(_T("Collecting System Data ..."));
		gui.DisplayResult(_T("System Data Capture: In Progress"));
        ClearSysCallsLog();
		GetSystemData(repository);
        ReportSysCalls("System Data");
		gui.DisplayResult(_T("System Data Capture: Complete"));
	}
	

	// Load common resources struct.  All modules should use this.
	//initCommonResources(commonResources, gui, AppLog, repository, g_osvex, config);
	commonResources.gui = gui;
	commonResources.AppLog = AppLog;
	commonResources.repository = repository;
	commonResources.osvex = g_osvex;
	commonResources.config = config;
	
	//Print the memory -> file mapping into the repository (only for 64bit memory capture)
	if (config.getPhysicalMemory)
    {
		gui.DisplayStatus(_T("Writing physical memory map to the repository file"));
		if(memory_region_file_map != 0)
		{
			repository.OpenNode("PHYMEMMAP");
			for(int i=0;;i++)
			{
				TCHAR files[120];//30+30+30+30 - for each fileName array in the struct + 1
				memset(files,0,120);
	
				repository.OpenNode("Region");

				memset(g_tempBuf,0,256);
				_sntprintf_s(g_tempBuf,256,256,L"0x%08X`%08X",memory_region_file_map[i].start.HighPart,memory_region_file_map[i].start.LowPart);
				repository.WriteNodeWithValue("RegionBegin",g_tempBuf);
				
				memset(g_tempBuf,0,256);
				_sntprintf_s(g_tempBuf,256,256,L"0x%08X`%08X",memory_region_file_map[i].end.HighPart,memory_region_file_map[i].end.LowPart);
				repository.WriteNodeWithValue("RegionEnd",g_tempBuf);

				memset(g_tempBuf,0,256);
				_sntprintf_s(g_tempBuf,256,256,L"0x%08X`%08X",memory_region_file_map[i].size.HighPart,memory_region_file_map[i].size.LowPart);
				repository.WriteNodeWithValue("RegionSize",g_tempBuf);


				PCSTR type;
				switch (memory_region_file_map[i].type)
				{
				case 1: 
					{
						type = "OS Memory";
						break;
					}
				case 2:
					{
						type = "Reserved";
						break;
					}
				case 3:
					{
						type = "ACPI Reclaim";
						break;
					}
				case 4:
					{
						type = "ACPI NVS";
						break;
					}
				default:
					{
						type = "Undefined";
						break;
					}
				}

				repository.WriteNodeWithValue("RegionType",type);


				if(memory_region_file_map[i].type == 1)
				{
					_tcsncpy(files,memory_region_file_map[i].fileName[0],_tcslen(memory_region_file_map[i].fileName[0]));
					_tcsncat(files,L" ",1);
					_tcsncat(files,memory_region_file_map[i].fileName[1],_tcslen(memory_region_file_map[i].fileName[1]));
					_tcsncat(files,L" ",1);
					_tcsncat(files,memory_region_file_map[i].fileName[2],_tcslen(memory_region_file_map[i].fileName[2]));

					repository.WriteNodeWithValue("FileName",files);
				}
				
				
				repository.CloseNode("Region");

				if(memory_region_file_map[i].final == true)
					break;
			}
			repository.CloseNode("PHYMEMMAP");
		}

	}
	//Print the memory -> file mapping into the repository (only for 64 bit memory capture)

	if(config.getStorageDevices)
    {
		gui.DisplayState(_T("Disk Information"));
		gui.DisplayStatus(_T("Collecting Disk Information"));
		gui.DisplayResult(_T("Disk Information Capture: In Progress"));
        ClearSysCallsLog();
		GetDiskInfo(repository);
        ReportSysCalls("Disk Information Collection");
		gui.DisplayResult(_T("Disk Information Capture: Complete"));
    }

	// This section extracts all the extensions that are specified in the ScanConfig.xml file
	// The section was moved up here so that it can be used commonly across MRU//MRD//Collected files
	// code sections.

BEGIN

	gui.DisplayStatus(_T("Loading File extensions to search for"));
	map<wstring,int> extensionList;

	for(map<tstring, vector<tstring > >::iterator it=config.fileExtractionMap.begin();it != config.fileExtractionMap.end();it++)
	{	
		for(vector<tstring>::iterator vit=config.fileExtractionMap[(*it).first].begin();vit<config.fileExtractionMap[(*it).first].end();vit++)
		{
			extensionList[*vit]=1;
		}
	}

	vector<tstring> drives;
	vector<tstring>::iterator it;

	TCHAR buf[1024];
	TCHAR drive[4];
	memset(buf,0,1024);
	memset(drive,0,4);

	gui.DisplayStatus(_T("Analyzing drives for file search"));
	INC_SYS_CALL_COUNT(GetLogicalDriveStrings); // Needed to be INC TKH
	int ret = GetLogicalDriveStrings(sizeof(buf),buf);

	for(int i=0;i<ret;i=i+4)
	{
		memset(drive,0,sizeof(drive));
		_tcsncpy_s(drive,4,(buf+i),_TRUNCATE);

		INC_SYS_CALL_COUNT(GetDriveType); // Needed to be INC TKH
#ifdef PRO
		if( !_tcsncmp(drive,uslattDrive,3) || !_tcsncmp(drive,caseDrive,3))//Make sure we donot add the case storage drive as well as the us-latt drive during file capture.
		{
			continue;
		}
#endif
		switch(GetDriveType(drive))
		{
		case DRIVE_FIXED:
			if(config.getLiveFileSystem) //First do this check then exit if true.
			{
				wstring szVolume;
				if(IsTrueCryptDrive(drive[0],szVolume))
				{
					AppLog.Write("Adding Mounted TrueCrypt drive",WSLMSG_STATUS); 
					drives.push_back(drive);
					break;
				}
				else
				{
					gui.DisplayStatus(_T("NOTE: TrueCrypt drives found. Mounted TrueCrypt Drives have not been selected for file acquisition")); 
				}
			}
			if(config.getFixedMedia)
			{
				AppLog.Write("Adding FixedMedia to search drives ",WSLMSG_STATUS); 
				drives.push_back(drive);
				break;
			}
			break;

		case DRIVE_REMOVABLE:
			if(config.getRemovableMedia)
			{
				AppLog.Write("Adding RemovableMedia to search drives ",WSLMSG_STATUS); 
				drives.push_back(drive);
				break;
			}		
			break;
		case DRIVE_REMOTE:
			if(config.getNetworkMedia)
			{
				AppLog.Write("Adding NetworkMedia to search drives ",WSLMSG_STATUS); 
				drives.push_back(drive);
				break;
			}
			break;
		default:
			break;
		};
	}

END


	//registryMRU is new for unifying the way file capture filters are applied to all file capture locations.
    if(process_registry) // In order to collect files from MRU keys under registry the Registry collection option  
    {
		gui.DisplayState(_T("Registry Collection"));
		gui.DisplayStatus(_T("Starting Registry Collection ..."));
		gui.DisplayResult(_T("Registry Processing: In Progress"));
        ClearSysCallsLog();
		
		GetRegistry(repository,drives,extensionList);
        ReportSysCalls("Registry Collection");
		gui.DisplayResult(_T("Registry Processing: Complete"));
    }

	if (config.getFullScreen || config.getDesktop || config.getIndividualWindows)
    {
		gui.DisplayState(_T("Screenshot Capture"));
		gui.DisplayStatus(_T("Starting  Screen Capture ..."));
		gui.DisplayResult(_T("Screen/Window Capture: In Progress"));
        ClearSysCallsLog();
		CaptureScreenshot(config.getFullScreen,config.getDesktop,config.getIndividualWindows);
        ReportSysCalls("Screen Capture");
		gui.DisplayResult(_T("Screen/Window Capture: Complete"));
    }

	if(config.getApplication)
    {
		gui.DisplayState(_T("Installed Applications"));
		gui.DisplayStatus(_T("Collecting Installed Application Information ..."));
		gui.DisplayResult(_T("Application Capture: In Progress"));
        ClearSysCallsLog();
		GetInstalledAppInfo(repository);
        ReportSysCalls("Installed Programs");
		gui.DisplayResult(_T("Application Capture: Complete"));
    }

	
	if(process_eventlogs)
    {
		gui.DisplayState(_T("EventLog Capture"));
		gui.DisplayStatus(_T("Starting EventLog parsing ..."));
		gui.DisplayResult(_T("EventLog Capture: In Progress"));
        ClearSysCallsLog();
		GetEventLogPaths(repository);
        ReportSysCalls("Event Logs");
		gui.DisplayResult(_T("EventLog Capture: Complete"));
    }
	

	
#ifdef RECENTDOCS
	if(config.getMRD)
	{
		gui.SetPercentStatusBar(10,0,100);
		gui.DisplayState(_T("File Collection"));	
		gui.DisplayStatus(_T("Copying Files from My Recent Documents ..."));
		gui.DisplayResult(_T("My Recent Documents: In Progress"));
		AppLog.Write("Capturing My Recent documents",WSLMSG_STATUS);
		map<wstring,int >::iterator eit;
		vector<wstring> extensions;
		for(eit=(extensionList).begin();eit!=(extensionList).end();eit++)
		{
			extensions.push_back((*eit).first);
		}
		//Collecting My Recent Documents
		
		//getRecentDocs(getCurrentUserOnly,getAllUsers,followNw,followLocal,followRemovable);
		getRecentDocs(config.getrdActive,config.getrdAll,config.getFollowNetwork,config.getFollowLocal,config.getFollowRemovable,extensions);
		SendMessage(gui.hProgress, PBM_SETPOS, 100, 0); //Complete
		gui.DisplayResult(_T("My Recent Documents: Complete"));
	}
	SendMessage(gui.hProgress, PBM_SETPOS, 0, 0); //Complete
#endif

	if(allFiles)
	{
		gui.DisplayState(_T("File Collection"));
		gui.DisplayStatus(_T("Copying Files from internal/external drives ..."));
		gui.DisplayResult(_T("File Capture: In Progress"));
		startSearch(drives,extensionList);
		gui.DisplayResult(_T("File Capture: Complete"));
	}

		//Now collect the Most recently viewed image file


#ifndef NETFORENSICS	
	if (config.getIPConfiguration || config.getNetStat || config.getARPTable) 
	{
		gui.DisplayState(_T("Network Information"));
		gui.DisplayStatus(_T("Collecting network/port information ..."));
		gui.DisplayResult(_T("Network Information: In Progress"));
		repository.OpenNode("NetworkInfo");
		ClearSysCallsLog();
		GetNetworkInfo(repository);
		//getWiFiInfo(repository);
		repository.CloseNode("NetworkInfo");
		ReportSysCalls("Network Info");
		gui.DisplayResult(_T("Network Information: Complete"));
	}
#endif

#ifdef NETFORENSICS	
	if (config.getIPConfiguration || config.getNetStat || config.getARPTable) 
	{
		gui.DisplayState(_T("Network Information"));
		gui.DisplayStatus(_T("Collecting network/port information ..."));
		gui.DisplayResult(_T("Network Information: In Progress"));
		repository.OpenNode("NetworkInfo");
		//ClearSysCallsLog();
		doNetForensics(commonResources);

		repository.CloseNode("NetworkInfo");
		//ReportSysCalls("Network Info");
		gui.DisplayResult(_T("Network Information: Complete"));
	}
#endif

	if(currentState != ABORTING && currentState != ABORTED)
	{
		gui.DisplayState(_T("USB Device History"));
		gui.DisplayStatus(_T("Collecting USB Device history information ..."));
		gui.DisplayResult(_T("USB Information: In Progress"));
		//New features for version 1.4.0
#ifdef USBHISTORY
		GetUSBDeviceHistoryFromMachine(repository);
#endif
		gui.DisplayResult(_T("USB Information: In Progress"));

		
#ifdef TRUECRYPT
		if(config.getAcquireTruecryptImage)
		{
			gui.DisplayState(_T("TrueCrypt Imaging"));
			gui.DisplayStatus(_T("Commencing Imaging TrueCrypt Volume ..."));
			gui.DisplayResult(_T("TrueCrypt Imaging: In Progress"));
			ImageTrueCryptDrives(repository);//Check return value for error TODO
			gui.DisplayResult(_T("TrueCrypt Imaging: Complete"));
			gui.ResetProgressBar();
		}
#endif
       //New feature for version 1.5.0
#ifdef MAILFILECAPTURE
	   if(config.getCaptureMailFiles)
	   {
			gui.DisplayState(_T("Mail Files Capture"));
			gui.DisplayStatus(_T("Commencing Mail Files Capture ..."));
			gui.DisplayResult(_T("Mail Files: In Progress"));
			CaptureMail();
			gui.DisplayResult(_T("Mail Files: Complete"));
	   }
#endif
	}

#ifdef WEBFORENSICS//Release 1.6.0 captures firefox and IE history
	if(config.getWebHistory)
	{
		gui.DisplayState(_T("Web Activity"));
		gui.DisplayStatus(_T("Commencing web activity capture ..."));
		gui.DisplayResult(_T("Web Forensics: In Progress"));
		ExtractWebActivity(AppLog);
		gui.DisplayResult(_T("Web Forensics: Complete"));
	}
#endif
	
#ifdef TIMELINE//Release 1.6.0 captures startup/shutdown events from event log
	if(config.getActionTimeLine)
	{
		gui.DisplayState(_T("System Log Events"));
		gui.DisplayStatus(_T("Commencing event capture ..."));
		gui.DisplayResult(_T("Event Capture: In Progress"));
		ExtractStartupShutdown();
		gui.DisplayResult(_T("Event Capture: Complete"));
	}
#endif
#ifdef DRIVE_IMAGING
		ImageSelectedExternalDrives(repository); //Perform Drive imaging on external drives. Added with the release of US-LATT Pro 2.1.0 --RJM
#endif
#ifdef STRIKE
    repository.CloseNode("STRIKELIVE_Repository");
#else
	repository.CloseNode("USLATT_Repository");
#endif

    repository.Close(m_repositoryHash);

	gui.DisplayState(_T("Computing Hashes"));
	gui.DisplayStatus(_T("Calculating digest.xml ..."));
	gui.DisplayResult(_T("Hash Computation: In Progress"));
	AppLog.Write("Computing hash results.xml",WSLMSG_STATUS);
	
	MoveAndHashResults();	
	gui.DisplayResult(_T("Hash Computation: Complete"));

	AppLog.Write("Cleanup temporary files",WSLMSG_STATUS);
	memset(g_tempBuf,0,MAX_PATH);
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH,g_tempBuf);
	
	AppLog.Write("Acquisition and Triage Completed.", WSLMSG_STATUS);
	
	if(currentState == ABORTING)
    {
		currentState = ABORTED;
    }
	else
    {
		currentState = SCAN_COMPLETE;
    }
	cout << "WST common destroyed" << endl;
	AppLog.Write("Remote agent successfully terminated.", WSLMSG_STATUS);		
}
//-->RJM Collecting Network Drive 
BOOL WINAPI EnumerateFunc(HashedXMLFile &repository, LPNETRESOURCE lpnr, bool bOriginalCall)
{
    static int iCount=0;
    DWORD dwResult, dwResultEnum;
    HANDLE hEnum;
    DWORD cbBuffer = 16384;     // 16K is a good size
    DWORD cEntries = -1;        // enumerate all possible entries
    LPNETRESOURCE lpnrLocal;    // pointer to enumerated structures
    DWORD i;

	
    // Call the WNetOpenEnum function to begin the enumeration.

    INC_SYS_CALL_COUNT(WNetOpenEnum); 
    dwResult = WNetOpenEnum(RESOURCE_CONNECTED, // all network resources
                            RESOURCETYPE_ANY,   // all resources
                            0,  // enumerate all resources
                            lpnr,       // NULL first time the function is called
                            &hEnum);    // handle to the resource

    if (dwResult != NO_ERROR) 
    {
        return FALSE;
    }
    INC_SYS_CALL_COUNT(GlobalAlloc); 
    lpnrLocal = (LPNETRESOURCE) GlobalAlloc(GPTR, cbBuffer);
    if (lpnrLocal == NULL) 
    {
        return FALSE;
    }

    do 
    {
        INC_SYS_CALL_COUNT(ZeroMemory); 
        ZeroMemory(lpnrLocal, cbBuffer);

        // Call the WNetEnumResource function to continue
        //  the enumeration.
        INC_SYS_CALL_COUNT(WNetEnumResource); 
        dwResultEnum = WNetEnumResource(hEnum,  // resource handle
                                        &cEntries,      // defined locally as -1
                                        lpnrLocal,      // LPNETRESOURCE
                                        &cbBuffer);     // buffer size
        // If the call succeeds, loop through the structures.
        if (dwResultEnum == NO_ERROR) 
        {
            for (i = 0; i < cEntries; i++) 
            {

                // Call an application-defined function to
                //  display the contents of the NETRESOURCE structures.
                iCount++;
                AddStructToXML(repository,i, &lpnrLocal[i]);
                if (RESOURCEUSAGE_CONTAINER == (lpnrLocal[i].dwUsage
                                                & RESOURCEUSAGE_CONTAINER))
                {
                    if (!EnumerateFunc(repository, &lpnrLocal[i], false))
                    {
                    }
                }
            }
        }
        // Process errors.
        else if (dwResultEnum != ERROR_NO_MORE_ITEMS) 
        {
            break;
        }
    }
    while (dwResultEnum != ERROR_NO_MORE_ITEMS);

    if (bOriginalCall)
    {
        char szCount[10];
        sprintf_s(szCount, sizeof(szCount), "%d", iCount);
        repository.WriteNodeWithValue("Count", szCount);
    }

    INC_SYS_CALL_COUNT(GlobalFree); 
    GlobalFree((HGLOBAL) lpnrLocal);
    // Call WNetCloseEnum to end the enumeration.
    INC_SYS_CALL_COUNT(WNetCloseEnum); 
    dwResult = WNetCloseEnum(hEnum);

    if (dwResult != NO_ERROR) {
        return FALSE;
    }

    return TRUE;
}

bool WSTRemote::GetDriveGeo(int iDriveNumber, HashedXMLFile &repository)
{
    unsigned char buffer[1000];
	DISK_GEOMETRY_EX* pDiskGeoEx			  = NULL;	// Disk Geometry
	DISK_GEOMETRY     diskGeo;	// Disk Geometry
	DISK_DETECTION_INFO* pBIOSDiskInfo		  = NULL;	// Int13 info (if any)
	DISK_PARTITION_INFO* pPartitionInfo		  = NULL;	// Partition table info
	DRIVE_LAYOUT_INFORMATION_EX* pDriveLayout = NULL;	// Variable sized array.

	unsigned int IOBytes	     = 0;
	unsigned int PDL_Size		 = 0;
	unsigned int PTBL_Size		 = 0;
	unsigned int ActualPartition = 0;
	unsigned int DGE_Size		 = 0;
	HANDLE DiskHandle			 = NULL;
	TCHAR DrivePath[MAX_PATH]	 = TEXT("");
    LARGE_INTEGER capacity;

	_stprintf_s(DrivePath, MAX_PATH, TEXT("\\\\.\\PhysicalDrive%d"), iDriveNumber);
    // attempt to open the drive
    INC_SYS_CALL_COUNT(CreateFile); 
	DiskHandle = CreateFile(DrivePath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(DiskHandle == INVALID_HANDLE_VALUE)
	{
        //AppLog.Write("Unable to open physical disk", WSLMSG_STATUS);
        //AppLog.Write("Some disk information could not be collected.", WSLMSG_STATUS);
		return false;
	}

    // we do have a drive, lets record it.
    repository.OpenNodeWithAttributes("PhysicalDisk");
	_itot_s(iDriveNumber, g_tempBuf, MAX_PATH, 10);
    repository.WriteAttribute("DriveNum", g_tempBuf);
    repository.Write(">");

    // BUS TYPE
	DWORD Bret;
	STORAGE_PROPERTY_QUERY query;
	query.PropertyId=StorageDeviceProperty;
	query.QueryType=PropertyStandardQuery;
    INC_SYS_CALL_COUNT(DeviceIoControl); 
	if(DeviceIoControl(DiskHandle,
                       IOCTL_STORAGE_QUERY_PROPERTY,
                       &query, sizeof(query),
                       buffer,
                       sizeof(buffer),
                       &Bret,
                       NULL))
	{
		STORAGE_DEVICE_DESCRIPTOR *desc=(STORAGE_DEVICE_DESCRIPTOR *) buffer;
        if(desc->BusType < BusTypeMax)
        {
            repository.WriteNodeWithValue("BusType", g_szBuses[desc->BusType]);
        }
		else
        {
            repository.WriteNodeWithValue("BusType", "unknown");
        }
	}

    // SERIAL NUMBER
    ATASERIAL driveSerial;
    INC_SYS_CALL_COUNT(GetDriveSerialATAFromHandle); 
    if (GetDriveSerialATAFromHandle(DiskHandle, &driveSerial))
    {
        repository.WriteNodeWithValue("ModelNumber", driveSerial.cszModelNumber);
        repository.WriteNodeWithValue("SerialNumber", driveSerial.cszSerialNumber);
        repository.WriteNodeWithValue("Firmware", driveSerial.cszFirmwareRev);
    }

	// Should validate that pDiskGeoEx was allocated.
	// ...

	// DISK INFORMATION
    if (g_osvex.dwMajorVersion == 5 && g_osvex.dwMinorVersion == 0)
    {
        DGE_Size = sizeof(diskGeo);
        // IOCTL_DISK_GET_DRIVE_GEOMETRY_EX does not work in 2000, so use IOCTL_DISK_GET_DRIVE_GEOMETRY
        INC_SYS_CALL_COUNT(DeviceIoControl); 
	    if(DeviceIoControl(DiskHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, 
                           &diskGeo, DGE_Size, (LPDWORD)&IOBytes, NULL))
	    {
		    // Drive capacity
            capacity.QuadPart = diskGeo.Cylinders.QuadPart * diskGeo.TracksPerCylinder;
            capacity.QuadPart = capacity.QuadPart * diskGeo.SectorsPerTrack;
            capacity.QuadPart = capacity.QuadPart * diskGeo.BytesPerSector;
		    _i64tot_s(capacity.QuadPart, g_tempBuf, MAX_PATH, 10);
            repository.WriteNodeWithValue("Capacity", g_tempBuf);		

		    // Parse out base drive geometry
		    _itot_s(diskGeo.BytesPerSector, g_tempBuf, MAX_PATH, 10);
            repository.WriteNodeWithValue("SectorSize", g_tempBuf);
		    _i64tot_s(diskGeo.TracksPerCylinder, g_tempBuf, MAX_PATH, 10);
            repository.WriteNodeWithValue("Tracks", g_tempBuf);
            _i64tot_s(diskGeo.Cylinders.QuadPart, g_tempBuf, MAX_PATH, 10);
            repository.WriteNodeWithValue("Cylinders", g_tempBuf);
            _i64tot_s(diskGeo.SectorsPerTrack, g_tempBuf, MAX_PATH, 10);
            repository.WriteNodeWithValue("Sectors", g_tempBuf);
	    }
        else
        {
            AppLog.Write("Unable query physical disk.", WSLMSG_STATUS);
            AppLog.Write("Some disk information could not be collected.", WSLMSG_STATUS);
        }
    }
    else
    {
	    DGE_Size = sizeof(DISK_GEOMETRY_EX) + sizeof(DISK_PARTITION_INFO) + sizeof(DISK_DETECTION_INFO);
	    pDiskGeoEx = (DISK_GEOMETRY_EX*) new unsigned char[DGE_Size];
        INC_SYS_CALL_COUNT(SecureZeroMemory); 
	    SecureZeroMemory(pDiskGeoEx, DGE_Size);

        INC_SYS_CALL_COUNT(DeviceIoControl); 
	    if(DeviceIoControl(DiskHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, 
                           pDiskGeoEx, DGE_Size, (LPDWORD)&IOBytes, NULL))
	    {
		    // Drive capacity
		    _i64tot_s(pDiskGeoEx->DiskSize.QuadPart, g_tempBuf, MAX_PATH, 10);
            repository.WriteNodeWithValue("Capacity", g_tempBuf);		

		    // Parse out base drive geometry
		    _itot_s(pDiskGeoEx->Geometry.BytesPerSector, g_tempBuf, MAX_PATH, 10);
            repository.WriteNodeWithValue("SectorSize", g_tempBuf);
		    _i64tot_s(pDiskGeoEx->Geometry.TracksPerCylinder, g_tempBuf, MAX_PATH, 10);
            repository.WriteNodeWithValue("Tracks", g_tempBuf);
            _i64tot_s(pDiskGeoEx->Geometry.Cylinders.QuadPart, g_tempBuf, MAX_PATH, 10);
            repository.WriteNodeWithValue("Cylinders", g_tempBuf);
		    _i64tot_s(pDiskGeoEx->Geometry.SectorsPerTrack, g_tempBuf, MAX_PATH, 10);
            repository.WriteNodeWithValue("Sectors", g_tempBuf);

		    // BIOS Disk Info
            INC_SYS_CALL_COUNT(DiskGeometryGetDetect); 
		    pBIOSDiskInfo = (DISK_DETECTION_INFO*) DiskGeometryGetDetect(pDiskGeoEx);

		    if(pBIOSDiskInfo->DetectionType == DetectExInt13)
		    {
		        _i64tot_s(pBIOSDiskInfo->ExInt13.ExCylinders, g_tempBuf, MAX_PATH, 10);
                repository.WriteNodeWithValue("PhysicalCylinders", g_tempBuf);
		        _i64tot_s(pBIOSDiskInfo->ExInt13.ExHeads, g_tempBuf, MAX_PATH, 10);
                repository.WriteNodeWithValue("PhysicalHeads", g_tempBuf);
		        _i64tot_s(pBIOSDiskInfo->ExInt13.ExSectorsPerDrive, g_tempBuf, MAX_PATH, 10);
                repository.WriteNodeWithValue("PhysicalSectors", g_tempBuf);
		        _itot_s((int)pBIOSDiskInfo->ExInt13.ExSectorSize, g_tempBuf, MAX_PATH, 10);
                repository.WriteNodeWithValue("PhysicalSectorSize", g_tempBuf);
		        _i64tot_s(pBIOSDiskInfo->ExInt13.ExSectorsPerTrack, g_tempBuf, MAX_PATH, 10);
                repository.WriteNodeWithValue("PhysicalSectorsPerTrack", g_tempBuf);
		    }
    		
		    if(pBIOSDiskInfo->DetectionType == DetectInt13)
		    {
		        _i64tot_s(pBIOSDiskInfo->Int13.MaxCylinders, g_tempBuf, MAX_PATH, 10);
                repository.WriteNodeWithValue("PhysicalCylinders", g_tempBuf);
		        _i64tot_s(pBIOSDiskInfo->Int13.MaxHeads, g_tempBuf, MAX_PATH, 10);
                repository.WriteNodeWithValue("PhysicalHeads", g_tempBuf);
		        _i64tot_s(pBIOSDiskInfo->Int13.MaxCylinders * pBIOSDiskInfo->Int13.MaxHeads * pBIOSDiskInfo->Int13.SectorsPerTrack, 
                          g_tempBuf, MAX_PATH, 10);
                repository.WriteNodeWithValue("PhysicalSectors", g_tempBuf);

                repository.WriteNodeWithValue("PhysicalSectorSize",_T("512"));

		        _i64tot_s(pBIOSDiskInfo->Int13.SectorsPerTrack, g_tempBuf, MAX_PATH, 10);
                repository.WriteNodeWithValue("PhysicalSectorsPerTrack", g_tempBuf);
		    }		
	    }
        else
        {
            AppLog.Write("Unable query physical disk.", WSLMSG_STATUS);
            AppLog.Write("Some disk information could not be collected.", WSLMSG_STATUS);
        }
    }

	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	SecureZeroMemory(pDriveLayout, PDL_Size);

    IOBytes = 0;
    if (g_osvex.dwMajorVersion == 5 && g_osvex.dwMinorVersion == 0)
    {
        DRIVE_LAYOUT_INFORMATION *pDriveL;
	    PDL_Size = sizeof(DRIVE_LAYOUT_INFORMATION) + (4 * sizeof(PARTITION_INFORMATION));
	    pDriveL = (DRIVE_LAYOUT_INFORMATION *) new unsigned char[PDL_Size];
	    // This really needs to be dynamically setup to requery with a larger buffer
        INC_SYS_CALL_COUNT(DeviceIoControl); 
	    if(DeviceIoControl(DiskHandle, IOCTL_DISK_GET_DRIVE_LAYOUT, NULL, 0, pDriveL, PDL_Size, (LPDWORD)&IOBytes, NULL))
	    {
		    // Determine exactly how many partitions are there (MBR style)
		    for(unsigned int i = 0; i < pDriveL->PartitionCount; i++)
		    {
                if(pDriveL->PartitionEntry[i].PartitionType != PARTITION_ENTRY_UNUSED)
                {
				    ActualPartition++;

                    repository.OpenNode("Partition");
			        _i64tot_s(pDriveL->PartitionEntry[i].StartingOffset.QuadPart, g_tempBuf, MAX_PATH, 10);
                    repository.WriteNodeWithValue("StartSector", g_tempBuf);

			        _i64tot_s(pDriveL->PartitionEntry[i].PartitionLength.QuadPart, g_tempBuf, MAX_PATH, 10);
                    repository.WriteNodeWithValue("Length", g_tempBuf);

			        _itot_s(pDriveL->PartitionEntry[i].PartitionType, g_tempBuf, MAX_PATH, 10);
                    repository.WriteNodeWithValue("Type", g_tempBuf);

                    repository.CloseNode("Partition");
                }
		    }
        }
        else
        {
            AppLog.Write("Unable query drive layout.", WSLMSG_STATUS);
            AppLog.Write("Some disk information could not be collected.", WSLMSG_STATUS);
        }
    }
    else
    {
	    // Set up a partition layout starting with 4 partitions (max for AT BIOS)
	    PDL_Size = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (4 * sizeof(PARTITION_INFORMATION_EX));
	    pDriveLayout = (DRIVE_LAYOUT_INFORMATION_EX*) new unsigned char[PDL_Size];
	    IOBytes = 0;
	    // This really needs to be dynamically setup to requery with a larger buffer
        INC_SYS_CALL_COUNT(DeviceIoControl); 
	    if(DeviceIoControl(DiskHandle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, pDriveLayout, PDL_Size, (LPDWORD)&IOBytes, NULL))
	    {
		    // Determine exactly how many partitions are there (MBR style)
		    for(unsigned int i = 0; i < pDriveLayout->PartitionCount; i++)
		    {
			    if(pDriveLayout->PartitionEntry[i].Mbr.PartitionType != PARTITION_ENTRY_UNUSED)
                {
				    ActualPartition++;
                    repository.OpenNode("Partition");
			        _i64tot_s(pDriveLayout->PartitionEntry[i].StartingOffset.QuadPart, g_tempBuf, MAX_PATH, 10);
                    repository.WriteNodeWithValue("StartSector", g_tempBuf);

			        _i64tot_s(pDriveLayout->PartitionEntry[i].PartitionLength.QuadPart, g_tempBuf, MAX_PATH, 10);
                    repository.WriteNodeWithValue("Length", g_tempBuf);

			        _itot_s(pDriveLayout->PartitionEntry[i].Mbr.PartitionType, g_tempBuf, MAX_PATH, 10);
                    repository.WriteNodeWithValue("Type", g_tempBuf);

                    repository.CloseNode("Partition");
		        }
            }
	    }
        else
        {
            AppLog.Write("Unable query drive layout.", WSLMSG_STATUS);
            AppLog.Write("Some disk information could not be collected.", WSLMSG_STATUS);
        }
    }
    repository.CloseNode("PhysicalDisk");
    INC_SYS_CALL_COUNT(CloseHandle); 
	CloseHandle(DiskHandle);

	delete [] (unsigned char*)pDriveLayout;
	delete [] (unsigned char*)pDiskGeoEx;

    return true;
}

//********************************************************************************
//
//  GetDiskVolumes
//
//********************************************************************************
void WSTRemote::GetDiskVolumes(HashedXMLFile &repository)
{
	HANDLE VolumeHandle = NULL;
	int NumVolumes		= 0;
	BOOL bError = FALSE;
	DWORD dwCode = 0;
	TCHAR szVolume[MAX_PATH];
    string szTempVolID;
    wstring szVolFile;
	list<char>::iterator tcit;//=trueCryptDrives.begin();
	TCHAR szPath[MAX_PATH];
	

    INC_SYS_CALL_COUNT(FindFirstVolume); 
	VolumeHandle = FindFirstVolume(szVolume, MAX_PATH);
    if(VolumeHandle != INVALID_HANDLE_VALUE)
	{
        do 
        {
            unsigned int pos;
            tstring szVolumeGUID; 
	       
	        tstring szTempVolID;
	        unsigned int StrLen	   = 0;
	        unsigned int DriveType = 0;
	        int i = 0;
	        TCHAR tempDrive[MAX_PATH] = _T("c:\\");
	        TCHAR bufVol[MAX_PATH];
            TCHAR szFileSystem[MAX_PATH];
	        HMODULE hKernel = NULL;
            lpfnGetVolumePathNamesForVolumeName GetVolumePathNamesForVolumeName = NULL;
	        VOLUME_DISK_EXTENTS* VolExtents = NULL;
	        HANDLE VolumeHandle				= NULL;
	        unsigned int IOBytes			= 0;
	        int	IOReturn					= 0;

            repository.OpenNode("Volume");
			int trueCryptFlag =0;

            // extract the GUID
	        szVolumeGUID = szVolume;
	        pos = szVolumeGUID.find('{');
	        if (pos > 0 && pos < (szVolumeGUID.length()-1))
	        {
                szVolumeGUID = szVolumeGUID.substr(pos,szVolumeGUID.length() - pos);            
                szVolumeGUID.erase(szVolumeGUID.find('\\'), 1);
	        }

            szTempVolID = TEXT("\\\\?\\Volume");
	        szTempVolID += szVolumeGUID;
	        szTempVolID += _T("\\");
            repository.WriteNodeWithValue("GUID", szVolumeGUID.c_str());

            //szPath[0] = 0;
			memset(szPath,0,sizeof(szPath));
	        if(((g_osvex.dwMajorVersion == 5) && (g_osvex.dwMinorVersion > 0)) ||
                (g_osvex.dwMajorVersion > 5))
	        {
                INC_SYS_CALL_COUNT(LoadLibraryEx); 
		        hKernel = LoadLibraryEx(_T("kernel32.dll"), NULL, 0);
		        if(hKernel != NULL)
		        {
                    INC_SYS_CALL_COUNT(GetProcAddress); 
			        GetVolumePathNamesForVolumeName = (lpfnGetVolumePathNamesForVolumeName)GetProcAddress(hKernel, GVPNFVN);
		        }
                if (GetVolumePathNamesForVolumeName)
                {
                    INC_SYS_CALL_COUNT(GetVolumePathNamesForVolumeName); 
		            if (!GetVolumePathNamesForVolumeName((LPCTSTR)szVolume, (LPCH)szPath, sizeof(szPath), (DWORD*)&StrLen))
                    {
							;
                    }
                }
	        }
	        else
	        {
		        for(i = _T('c'); i <= _T('z'); i++)
		        {
			        memset(bufVol, 0, MAX_PATH * sizeof(TCHAR));

			        tempDrive[0] = i;
                    INC_SYS_CALL_COUNT(GetVolumeNameForVolumeMountPoint); 
			        GetVolumeNameForVolumeMountPoint(tempDrive, bufVol, MAX_PATH);
			        if(szTempVolID == bufVol)
			        {
				        _tcsncpy_s(szPath,tempDrive, _TRUNCATE);
				        break;
			        }
		        } 			
	        }
            if(_tcslen(szPath) > 0)
            {
				trueCryptFlag=0;
                repository.OpenNode("MountPoints");
                repository.WriteNodeWithValue("Name", szPath);
			    char dLet = (char)szPath[0];
                if ((dLet >= 'a' && dLet <= 'z') ||(dLet >= 'A' && dLet <= 'Z')) 
                {
                    if (config.getLiveFileSystems)
                    {
  			           AppLog.Write(szPath, WSLMSG_STATUS);
			           if (IsTrueCryptDrive(dLet, szVolFile)) 
                       {
						   TCHAR msg[40];
						   memset(msg,0,sizeof(TCHAR)*40);
                           tstring spath(szVolFile.begin(),szVolFile.end());
						   _stprintf(msg,_T("Found TrueCrypt Volume %s"),spath.c_str());
						   gui.DisplayStatus(msg);
//						   trueCryptDrives.insert(tcit,dLet);//Insert into drive list.
                           repository.WriteNodeWithValue("TrueCrypt", spath.c_str());
						   //lfs.push_back(szPath);
						   trueCryptFlag=1;
                       }
                       else 
                       {
                           if (IsFreeOTFEDrive(dLet, szVolFile))
                           {
								tstring spath(szVolFile.begin(),szVolFile.end());        
								TCHAR msg[40];
								memset(msg,0,sizeof(TCHAR)*40);
								_stprintf(msg,_T("Found FreeOTFEDrive Volume %s"),spath.c_str());
								gui.DisplayStatus(msg);
								repository.WriteNodeWithValue("FreeOTFE", spath.c_str());
                           }
                       }
					   
                    }
                }			
                repository.CloseNode("MountPoints");

	            // Might as well get the info on the volume while we're here.
                INC_SYS_CALL_COUNT(GetVolumeInformation); 
	            if (GetVolumeInformation(szPath, (LPTSTR)g_tempBuf, sizeof(g_tempBuf), NULL, NULL, NULL, (LPTSTR)szFileSystem, sizeof(szFileSystem)))
                {
                    repository.WriteNodeWithValue("VolName", g_tempBuf);
                    repository.WriteNodeWithValue("FileSystem", szFileSystem);

					//Collect all the drives except US-LATT

					
                }

                INC_SYS_CALL_COUNT(GetDriveType); 
 	            DriveType = GetDriveType(szPath);
				
	            switch(DriveType)
	            {
	            case DRIVE_REMOVABLE:
                    repository.WriteNodeWithValue("Type", "Removable");
					
					if(_tcscmp(g_tempBuf,_T("US-LATT")) && _tcscmp(g_tempBuf,_T("USLATT")) && _tcscmp(g_tempBuf,_T("LIVEU3")) && _tcscmp(g_tempBuf,_T("STRIKE")) )
					{
						//removable.push_back(szPath);
					}
		            break;
	            case DRIVE_FIXED:
					
					if(trueCryptFlag)
					{
						~-trueCryptFlag;
						break;
					}
					
					//fixed_drive.push_back(tstring(szPath));
					
                    repository.WriteNodeWithValue("Type", "Fixed Hard Disk");
		            break;
	            case DRIVE_REMOTE:
					
					//network.push_back(szPath);
                    repository.WriteNodeWithValue("Type", "Remote Storage (eg: Network)");
		            break;
	            case DRIVE_CDROM:
                    repository.WriteNodeWithValue("Type", "CD/DVD Drive");
		            break;
	            case DRIVE_RAMDISK:
                    repository.WriteNodeWithValue("Type", "RAMDisk");
		            break;
	            case DRIVE_NO_ROOT_DIR:
                    repository.WriteNodeWithValue("Type", "Unknown Drive Type");
		            break;
	            default:
                    repository.WriteNodeWithValue("Type", "Unknown Drive Type");
	            }
        	}

            // disk extents
            VolExtents = NULL;
	        szTempVolID = TEXT("\\\\?\\Volume");
	        szTempVolID += szVolumeGUID;
	        // Volume handles require share modes
            INC_SYS_CALL_COUNT(CreateFile); 
	        VolumeHandle = CreateFile(szTempVolID.c_str(), 
							          GENERIC_READ | GENERIC_WRITE,
							          FILE_SHARE_READ | FILE_SHARE_WRITE,
							          NULL,
							          OPEN_EXISTING,
							          0,
							          NULL);
	        // Test to ensure we have a good handle
	        if(VolumeHandle != INVALID_HANDLE_VALUE)
	        {
                // allocate memory for the structure
	            VolExtents = (VOLUME_DISK_EXTENTS*) new unsigned char[sizeof(VOLUME_DISK_EXTENTS)];
                INC_SYS_CALL_COUNT(SecureZeroMemory); 
	            SecureZeroMemory(VolExtents, sizeof(VOLUME_DISK_EXTENTS));
                INC_SYS_CALL_COUNT(DeviceIoControl); 
	            if (!DeviceIoControl(VolumeHandle, 
							               IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
							               NULL, 0,
							               VolExtents, sizeof(VOLUME_DISK_EXTENTS),
							               (LPDWORD)&IOBytes,
							               NULL))
	            {
		            // More than likely the device isn't supported, or it's a removable device
		            // reporting that it's not ready.
                    delete [] (unsigned char*)VolExtents;
                    VolExtents = NULL;
	            }
                else
                {
	                if(VolExtents->NumberOfDiskExtents > 1)
	                {
		                VOLUME_DISK_EXTENTS* TempVol = (VOLUME_DISK_EXTENTS*) new unsigned char[sizeof(VOLUME_DISK_EXTENTS) + (VolExtents->NumberOfDiskExtents * sizeof(DISK_EXTENT))];
                        INC_SYS_CALL_COUNT(SecureZeroMemory); 
		                SecureZeroMemory(TempVol, sizeof(VOLUME_DISK_EXTENTS) + (VolExtents->NumberOfDiskExtents * sizeof(DISK_EXTENT)));                		
		                delete [] (unsigned char*)VolExtents;
		                VolExtents = TempVol;
                        INC_SYS_CALL_COUNT(DeviceIoControl); 
		                IOReturn = DeviceIoControl(VolumeHandle, 
								                   IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
								                   NULL, 0,
								                   VolExtents, sizeof(VOLUME_DISK_EXTENTS),
								                   (LPDWORD)&IOBytes,
								                   NULL);

		                if(!IOReturn)
		                {
			                // throw(L"Error calling DeviceIoControl.");
			                delete [] (unsigned char*)VolExtents;
                            VolExtents = NULL;
		                }
	                }
                }

                INC_SYS_CALL_COUNT(CloseHandle); 
                CloseHandle(VolumeHandle);
                if (VolExtents)
                {
                    // We should have our volume extents now.
                    for(unsigned int i = 0; i < VolExtents->NumberOfDiskExtents; i++)
                    {
                        repository.OpenNodeWithAttributes("DiskExtent");
			            _itot_s(VolExtents->Extents[i].DiskNumber, g_tempBuf, MAX_PATH, 10);
                        repository.WriteAttribute("DriveNum", g_tempBuf);
                        repository.Write(">");
			            _i64tot_s(VolExtents->Extents[i].StartingOffset.QuadPart, g_tempBuf, MAX_PATH, 10);
                        repository.WriteNodeWithValue("Start", g_tempBuf);
			            _i64tot_s(VolExtents->Extents[i].ExtentLength.QuadPart, g_tempBuf, MAX_PATH, 10);
                        repository.WriteNodeWithValue("Length", g_tempBuf);
                        repository.CloseNode("DiskExtent");
                    }
                    delete [] (unsigned char*)VolExtents;
                }
	        }

            repository.CloseNode("Volume");
            INC_SYS_CALL_COUNT(FindNextVolume); 
        } while(FindNextVolume(VolumeHandle, szVolume, MAX_PATH) != 0);
        INC_SYS_CALL_COUNT(FindVolumeClose); 
        FindVolumeClose(VolumeHandle);
    }	
}


//FIXME -- Better error handling there is not error handling (part of this should be the function returning an int not void)

void WSTRemote::GetDiskInfo(HashedXMLFile &repository)
{
	unsigned int iNumDrives = 0;
	unsigned int iPartitions = 0;
	unsigned int iNumVolumes = 0;
//	DiskSubsys ds;
	tstring filename = _T("DiskInfo.xml");
	LPNETRESOURCE lpnr = NULL;

	gui.DisplayState(_T("Disk Information"));

	AppLog.Write("Collecting disk drive information.", WSLMSG_STATUS);

	// Regardless of what file we use, everything is going to be under a DiskInfo tag
    repository.OpenNode("DiskInfo");
	
	// Physical drive information
    repository.OpenNode("PhysicalDrives");
    // go through up to 128 different drives
	gui.DisplayStatus(_T("Computing drive geometry."));
	for(unsigned int i = 0; i < 128; i++)
	{
		INC_SYS_CALL_COUNT(GetDriveGeo); // Needed to be INC TKH
        GetDriveGeo(i, repository);
	}
    repository.CloseNode("PhysicalDrives");

    repository.OpenNode("DiskVolumes");		
	INC_SYS_CALL_COUNT(GetDiskVolumes); // Needed to be INC TKH
	gui.DisplayStatus(_T("Collecting Disk Volume information"));
    GetDiskVolumes(repository);
    repository.OpenNode("NetworkDrives");
    if (EnumerateFunc(repository, lpnr, true) == FALSE) 
    {
		return;
    }
    repository.CloseNode("NetworkDrives");
    repository.CloseNode("DiskVolumes");
    repository.CloseNode("DiskInfo");
	AppLog.Write("Done collecting disk drive information.", WSLMSG_STATUS);

	return;
}

tstring WSTRemote::XMLStringCheck(tstring stringToCheck)
{
	AppLog.Write("WSTRemote::XMLStringCheck(...)", WSLMSG_DEBUG);

	tstring safestr = TEXT("");
#ifdef UNICODE

	// I'm not sure what to do yet for unicode 
	safestr = stringToCheck;

#else
	TCHAR converted[4];  // 4 chars for ASCII code + null term 

	for (size_t i = 0; i < stringToCheck.length(); i++) 
    {
		if (stringToCheck[i] == 38 || !__isascii(stringToCheck[i])) 
        {
			// convert to text representation and put in the string
			_itoa_s((unsigned char)stringToCheck[i], converted, 4, 10);
			safestr += TEXT("&#");
			safestr += converted;
			safestr += TEXT(';');
		}
		else 
        {
			// just dump into the string as is
			safestr += stringToCheck[i];
		}
	}

#endif

	return safestr;

}

int WSTRemote::CaptureScreenshot(int fullscreen,int desktop,int individual)   
{
    int iRetVal = 0;

	if((desktop == 0) && (fullscreen == 0) && (individual == 0))
		return NO_SCREEN_CAPTURE; //NO screenshot capture

	AppLog.Write("WSTRemote::CaptureScreenshot()", WSLMSG_DEBUG);

	SysData sd;
	tstring filename, msg;
    tstring szWindowLabel;

	if(g_osvex.dwMajorVersion >= 6)
	{
//		AppLog.Write("** Windows version 6.0 or higher (Vista/2008) detected.", WSLMSG_STATUS);
//		AppLog.Write("** Disabling screen capture.", WSLMSG_STATUS);
		//return WINDOWS_VERSION_ERROR;
	}

	//RJM - Changed the applog line.
#ifdef STRIKE
	AppLog.Write("Capturing screenshots.", WSLMSG_STATUS);
#else
	AppLog.Write("Capturing USLATT screenshots.", WSLMSG_STATUS);
#endif


	//RJM - Changed to include screen captures of individual windows along-with the 
	//entire screen.
	//Changed the number of arguments to GetScreenCaptures.
    szWindowLabel = _T("Removable Disk (");
	
#ifdef _UNICODE
	size_t len=0;
	len=mbstowcs(NULL,m_szCurrentDrive,0);

	wchar_t *wcd=new wchar_t[len];
	mbstowcs(wcd,m_szCurrentDrive,len);
	szWindowLabel.append(wcd);
	if(wcd)
	{
		delete [] wcd;
	}
#else
	szWindowLabel.append(m_szCurrentDrive);
#endif
    
    szWindowLabel = szWindowLabel + _T(":)");
	//Changed the first parameter to 0 to remove compression 
    if (sd.GetScreenCaptures(0,fullscreen,desktop,individual,iUseEncryption,const_cast<wchar_t *>(szWindowLabel.c_str())) == SysData::SUCCESS) 
	{
		AppLog.Write("Done capturing screenshots.", WSLMSG_STATUS);
	}
	else
		AppLog.Write("There has been an error capturing the screen shot.", WSLMSG_STATUS);

   
    return iRetVal;
}

bool WSTRemote::LoadConfigFile(tstring filename)
{
	AppLog.Write("WSTRemote::LoadConfigFile(...)", WSLMSG_DEBUG);

	XMLResults result;
	XMLNode value, child, parent;
	tstring configData;
	tstring err;
	

	_stprintf_s(g_tempBuf, MAX_PATH, _T("Reading triage configuration file: %s"), filename.c_str());
	AppLog.Write(g_tempBuf, WSLMSG_STATUS);
	gui.DisplayStatus(g_tempBuf);

	try
	{
		XMLNode root = XMLNode::parseFile(filename.c_str(), CONFIG_ROOT_NAME, &result);

		if (result.error != eXMLErrorNone) {
			err = TEXT("Loading configuration failed: ");
			err += XMLNode::getError(result.error);
			AppLog.Write("There was an error reading the configuration file.", WSLMSG_STATUS);
			return false;
		}

		// Nothing in the file...
		if(root.isEmpty())
		{
			AppLog.Write("No valid ScanConfig.xml file found.", WSLMSG_STATUS);
			return false;
		}

        config.getSysData = true;


		// a little shortcut
	#define READ_SCANCONFIG_BOOL(item) \
	value = child.getChildNode(_T(#item)); \
        if (!value.isEmpty()) { \
		configData = value.getText(); \
		configData == _T("0") ? config.get##item = false : config.get##item = true; \
        } else {config.get##item = false;} 

	#define READ_SCANCONFIG_BOOL_NODE(item,node) \
        value = node.getChildNode(_T(#item)); \
        if (!value.isEmpty()) { \
		configData = value.getText(); \
		configData == _T("0") ? config.get##item = false : config.get##item = true; \
        } else {config.get##item = false;} 
		
		child = root.getChildNode(_T("System"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) 
        {
            value = child.getChildNode(_T("UserAccounts"));
            if (!value.isEmpty())
            {
			    configData = value.getText();
			    if(configData != _T("0"))
				    config.getAllUsers = true;
            }

			READ_SCANCONFIG_BOOL(OperatingSystem);
			//READ_SCANCONFIG_BOOL(Patches); -- Removed for IDEAL work
			READ_SCANCONFIG_BOOL(MachineInformation);

			child = child.getChildNode(_T("Logs"));
			if (!child.isEmpty() && (child.nChildNode() > 0)) 
			{
				child = child.getChildNode(_T("Security"));
				if (!child.isEmpty() && (child.nChildNode() > 0))
				{
					config.getStartShutActions=(child.getChildNode(_T("StartUp-ShutdownActions"))).getText();
					config.getLoginLogoutActions=(child.getChildNode(_T("Login-LogoutActions"))).getText();
					READ_SCANCONFIG_BOOL(AccountActions);
					READ_SCANCONFIG_BOOL(FirewallActions);
					READ_SCANCONFIG_BOOL(MiscActions);
					process_eventlogs=true;
				}
			}
		}

        // I have no idea where/if this is already initialized MJD
		config.getLiveFileSystems = false;
		child = root.getChildNode(_T("LiveFileSystems"));
		if(!child.isEmpty())
		{
			configData = child.getText();
			if(configData != _T("0"))
				config.getLiveFileSystems = true;
		}

		child = root.getChildNode(_T("Hardware"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) {
			READ_SCANCONFIG_BOOL(Drivers);
			READ_SCANCONFIG_BOOL(StorageDevices);
		}

		child = root.getChildNode(_T("Network"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) {
			READ_SCANCONFIG_BOOL(IPConfiguration);
			READ_SCANCONFIG_BOOL(ARPTable);
			READ_SCANCONFIG_BOOL(NetStat);
		}

		child = root.getChildNode(_T("Processes"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) {
			READ_SCANCONFIG_BOOL(GetProcesses);
		}

		child = root.getChildNode(_T("MemoryCapture"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) {
			READ_SCANCONFIG_BOOL(PhysicalMemory);
//			READ_SCANCONFIG_BOOL(VirtualMemory);
		}
		
		child = root.getChildNode(_T("ScreenCapture"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) {
			READ_SCANCONFIG_BOOL(FullScreen);
			READ_SCANCONFIG_BOOL(Desktop);
			READ_SCANCONFIG_BOOL(IndividualWindows);
		//	READ_SCANCONFIG_BOOL(BrowserTabs);
		}


		child = root.getChildNode(_T("Services"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) {
			READ_SCANCONFIG_BOOL(ServiceStatus);
		}

		process_filecapture=false;
		child = root.getChildNode(_T("RecentlyUsedCapture"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) 
		{
			// Enabled is always set to 1 in the ScanConfig.xml.
			// This is a shortcut and not a good design.
			// redesign later.

			int enabled=_tatoi(child.getChildNode(_T("Enabled")).getText());

			// Added AllFiles tag to collect files from all media
			// This is for the call to the funtion in cfilefind.cpp

			allFiles = _tatoi(child.getChildNode(_T("AllFiles")).getText());

			// Added RegistryMRU tag to collect files from all media
			// This is for the call to the funtion in that collects files from MRU
			// in the registry

			config.registryMRU = _tatoi(child.getChildNode(_T("RegistryMRU")).getText());

			if(enabled!=0)
			{
				process_filecapture=true;
			}
			
			if(process_filecapture)
			{
				config.getPreviousDays = _tatoi(child.getChildNode(_T("PreviousDays")).getText());
				config_MRU_TIME = config.getPreviousDays;
				config.getMaximumFileSize = _tatol(child.getChildNode(_T("MaximumFileSize")).getText());
				config.modifiedDate = _tatoi(child.getChildNode(_T("ModifiedDate")).getText());
				config.createdDate = _tatoi(child.getChildNode(_T("CreatedDate")).getText());

				XMLNode grandchild = child.getChildNode(_T("Categories"));
				if (!grandchild.isEmpty() && (grandchild.nChildNode() > 0)) 
				{
					
					for(int countCat=0;countCat<grandchild.nChildNode();countCat++)
					{
						XMLNode extNode=grandchild.getChildNode(countCat); //Get the extension node. This is stupid.
						tstring category=extNode.getName();

						int totalExt=extNode.getChildNode(_T("Extensions")).nChildNode(); //The Extensions subnode is driving me crazy.
						vector<tstring> extList;

						for(int countExt=0;countExt<totalExt;countExt++)
						{
							//cout<<(extNode.getChildNode(_T("Extensions")).getChildNode(countExt)).getName()<<endl;
							extList.push_back(((extNode.getChildNode(_T("Extensions")).getChildNode(countExt)).getName()));
						}
						config.fileExtractionMap[category]=extList;
					}

				}
	
				grandchild = child.getChildNode(_T("SearchLocations"));
				if (!grandchild.isEmpty() && (grandchild.nChildNode() > 0))
				{
					READ_SCANCONFIG_BOOL_NODE(FixedMedia,grandchild);
					READ_SCANCONFIG_BOOL_NODE(RemovableMedia,grandchild);
					READ_SCANCONFIG_BOOL_NODE(NetworkMedia,grandchild);
					READ_SCANCONFIG_BOOL_NODE(LiveFileSystem,grandchild);
				}
			}

		}

		child = root.getChildNode(_T("Registry"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) 
		{
			
			XMLNode grandChild=child.getChildNode(_T("Enabled"));
			
				//int enabled =_tatoi(child.getChildNode(_T("Enabled")).getText());
				 TCHAR *enabled =const_cast<TCHAR *>( (child.getChildNode(_T("Enabled"))).getText());



				if((_tcsncmp(enabled,_T("0"),1)))
				{
					process_registry=true;
					XMLNode regKeyNode=child.getChildNode(_T("Keys"));
					if (!regKeyNode.isEmpty() && (regKeyNode.nChildNode() > 0))
					{

						READ_SCANCONFIG_BOOL_NODE(FolderSearch,regKeyNode);
						READ_SCANCONFIG_BOOL_NODE(FileSearch,regKeyNode);
						READ_SCANCONFIG_BOOL_NODE(RecentlyRun,regKeyNode);
						READ_SCANCONFIG_BOOL_NODE(RecentDocs,regKeyNode);

						/*
						int numOfKeys=regKeyNode.nChildNode();
						for(int i=0;i<numOfKeys;i++)
						{
							tstring keyStr=regKeyNode.getChildNode(i).getText();
							config.MapRegistryKey.push_back(keyStr);
						}
						*/
					}
				}
		}

#ifdef RECENTDOCS
		child = root.getChildNode(_T("MyRecentDocuments"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) 
		{
			
			XMLNode grandChild=child.getChildNode(_T("CaptureMRD"));	
			config.getMRD=(( _tatoi((child.getChildNode(_T("CaptureMRD"))).getText()) == 0 )?false:true);
			if(config.getMRD)
			{
				XMLNode users=child.getChildNode(_T("Users"));

				if(!users.isEmpty() && (users.nChildNode() > 0))
				{
					config.getrdActive=(( _tatoi((users.getChildNode(_T("Active"))).getText()) == 0 )?false:true);
					config.getrdAll=(( _tatoi(users.getChildNode(_T("All")).getText()) == 0 ) ? false:true);
				} 

				XMLNode targets=child.getChildNode(_T("Target"));
				if(!targets.isEmpty() && (targets.nChildNode() > 0))
				{
					config.getFollowLocal=((_tatoi(targets.getChildNode(_T("FollowLocal")).getText()) == 0 )?false:true);
					config.getFollowNetwork=((_tatoi(targets.getChildNode(_T("FollowNetwork")).getText()) == 0 )?false:true);
					config.getFollowRemovable=((_tatoi(targets.getChildNode(_T("FollowRemovable")).getText()) == 0 )?false:true);
				}


			}
		}

#endif
		
		child = root.getChildNode(_T("GeneralOptions"));
		if (!child.isEmpty() && (child.nChildNode() > 0)) 
		{
#ifdef TRUECRYPT
			config.getAcquireTruecryptImage=(( _tatoi((child.getChildNode(_T("AcquireTruecrypt"))).getText()) == 0 )?false:true);
		
#endif
#ifdef MAILFILECAPTURE
			config.getCaptureMailFiles=(( _tatoi((child.getChildNode(_T("CaptureMailFiles"))).getText()) == 0 )?false:true);
#endif
#ifdef WEBFORENSICS
			config.getWebHistory=(( _tatoi((child.getChildNode(_T("WebHistory"))).getText()) == 0 )?false:true);
#endif
#ifdef TIMELINE
			config.getActionTimeLine=(( _tatoi((child.getChildNode(_T("ActionsTimeLine"))).getText()) == 0 )?false:true);
#endif
		}


	/*	for(std::vector<string>::iterator it=config.MapRegistryKey.begin();it<config.MapRegistryKey.end();it++)
			cout<<(*it)<<endl;
*/
		child = root.getChildNode(_T("InstalledApps"));
		if(!child.isEmpty())
		{
			configData = child.getText();
			if(configData != _T("0"))
				config.getApplication = true;
		}
		
		AppLog.Write("Done reading triage configuration.", WSLMSG_STATUS);

		return true;
	}
	catch(...)
	{
		return false;
	}
}

void WSTRemote::ResetConfig()
{
	AppLog.Write("WSTRemote::ResetConfig", WSLMSG_DEBUG);

		//<System>
		 config.getOperatingSystem=false;
		 config.getMachineInformation=false;
		 config.getAllUsers=false; //There is a function GetLoggedinUsers but the corresponding config variable is never used so I have commented it out.
		//<LOGS>
			//<Security> //Replaces config.getEvent
				 config.getStartShutActions=false;
				 config.getLoginLogoutActions=false;
				 config.getAccountActions=false;
				 config.getFirewallActions=false;
				 config.getMiscActions=false;
			//</Security>
		//</LOGS>
	//</System>

	//<Hardware>
		 config.getDrivers=false;
		 config.getStorageDevices=false;
	//</Hardware>
	
	//<LiveFileSystems>
		 config.getLiveFileSystems=false;
	//</LiveFileSystems>

	//<Network>
		 config.getIPConfiguration=false; //This was p instead of P. I wonder how it was working before?
		 config.getARPTable=false;
		 config.getNetStat=false;
	//</Network>

	//<Processes>
		 config.getGetProcesses=false;
	//</Processes>

	//<MemoryCapture>
		 config.getPhysicalMemory=false;
	//</MemoryCapture>

	//<RecentlyUsedCapture>
		//<PreviousDays>
			 config.getPreviousDays=0;
		//</PreviousDays>
		//<MaximumFileSize>
			 config.getMaximumFileSize=1024*1024;
		//</MaximumFileSize>
		//<Categories>
			//<Documents>
				 //config.getDocuments=false;
			//</Documents>
			//<Images>
				 //config.getImages=false;
			//</Images>
			//<MultiMedia>
				 //config.getMultiMedia=false;
			//</MultiMedia>
		//</Categories>
		//<Extensions>
			//<doc>
				 //config.getdoc=false;
			//</doc>
			//<docx>
				 //config.getdocx=false;
			//</docx>
			//<wav>
				 //config.getwav=false;
			//</wav>
		//</Extensions>
		//<SearchLocations>
			//<FixedMedia>
				 config.getFixedMedia=false;
			//</FixedMedia>
			//<RemovableMedia>
				 config.getRemovableMedia=false;
			//</RemovableMedia>
			//<NetworkMedia>
				 config.getNetworkMedia=false;
			//</NetworkMedia>
			//<LiveFileSystem>
				 config.getLiveFileSystem=false;
		    //</LiveFileSystem>
		//</SearchLocations>
	//</RecentlyUsedCapture>

	//<ScreenCapture>
		//<FullScreen>
			 config.getFullScreen=false;
		//</FullScreen>
		//<Desktop>
			 config.getDesktop=false;
		//</Desktop>
		//<IndividualWindows>
			 config.getIndividualWindows=false;
		//</IndividualWindows>
		//<BrowserTabs>
				 //config.getBrowserTabs=false;
		//</BrowserTabs>
	//</ScreenCapture>
	//<Services>
		//<ServiceStatus>
			 config.getServiceStatus=false;
		//</ServiceStatus>
	//</Services>
	//<InstalledApps>
		 config.getApplication=false; //This variable is named wrongly. I won't correct it now since it would require looking at other places.
	//</InstalledApps>
	//<Registry>
		 /*
		//<OpenSaveMRU>
			 config.getOpenSaveMRU=false;
		//</OpenSaveMRU>
		//<RunMRU>
			 config.getRunMRU=false;
		//</RunMRU>
		//<LastVisitedMRU>
			 config.getLastVisitedMRU=false;
		//</LastVisitedMRU>
		//<RecentDocs>
			 config.getRecentDocs=false;
		//</RecentDocs>
		//<FolderSearchTerms>
			 config.getFolderSearchTerms=false;
		//</FolderSearchTerms>
		//<FileSearchTerms>
			 config.getFileSearchTerms=false;
		//</FileSearchTerms>
		//<Wireless>
			 config.getWireless=false;
		//</Wireless>
		*/
	//</Registry>

#ifdef RECENTDOCS
		config.getMRD=false;
		config.getrdActive=false;
		config.getrdAll=false;
		config.getFollowNetwork=false;
		config.getFollowLocal=false;
		config.getFollowRemovable=false;
#endif
	//<Registry>
		config.getFolderSearch=false;
		config.getFileSearch=false;
		config.getRecentlyRun=false;
		config.getRecentDocs=false;
		//std::vector<std::tstring> MapRegistryKey; No longer used, remove after 1.2.5
	//</Registry>

#ifdef TRUECRYPT
		config.getAcquireTruecryptImage=false;
#endif
#ifdef MAILFILECAPTURE
		config.getCaptureMailFiles=false;
#endif

#ifdef WEBFORENSICS
		config.getWebHistory=false;
#endif
#ifdef TIMELINE
			config.getActionTimeLine=false;
#endif
	 config.getSysData=false;//--Another stupidity.

}

//32 bit arch change
/*
//Return 1 for success, 3 for any stop related errors and 4 for service being already active -- RJM

ULONG UninstallPhyMemCaptureDriver(void)
{
	SC_HANDLE ServiceManager;
	SC_HANDLE Service;
	SERVICE_STATUS ServiceStatus;

    // Open service manager
    INC_SYS_CALL_COUNT(OpenSCManager); 
    ServiceManager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

    if (!ServiceManager)
	{
		AppLog.Write("Failed to uninstall Physical Memory capture driver (Service Manager)", WSLMSG_ERROR);
//		printf("Failed\n");
		return 3;
	}

    // Open win32dd service.
    INC_SYS_CALL_COUNT(OpenService); 
    Service = OpenService(ServiceManager, L"vista_memcap", SERVICE_ALL_ACCESS);


    if (!Service)
	{
        INC_SYS_CALL_COUNT(CloseServiceHandle); 
		CloseServiceHandle(ServiceManager);
		AppLog.Write("No service for Physical Memory capture present (OpenService)- In uninstall driver ", WSLMSG_STATUS);
		return 4;
	}

	//Debug to remove
	SERVICE_STATUS_PROCESS ssp;
	DWORD dwBytesNeeded=0;
    INC_SYS_CALL_COUNT(QueryServiceStatusEx); 
	if(!QueryServiceStatusEx(Service,SC_STATUS_PROCESS_INFO,(LPBYTE)&ssp,sizeof(SERVICE_STATUS_PROCESS),&dwBytesNeeded))
	{
		AppLog.Write("QueryServiceStatus Failed in UninstallPhyMemCaptureDriver", WSLMSG_ERROR);
	}
	

	//Debug to remove/

	SetLastError(0);
    // Stop our service.

	if(ssp.dwCurrentState != SERVICE_CONTROL_STOP)
    {
        INC_SYS_CALL_COUNT(ControlService); 
        if (!ControlService(Service, SERVICE_CONTROL_STOP, &ServiceStatus))
	    {
		    AppLog.Write("ControlService Failed in UninstallPhyMemCaptureDriver", WSLMSG_ERROR);
	    }
    }
    // Delete service.
    INC_SYS_CALL_COUNT(DeleteService); 
    if(!DeleteService(Service))
	{
		AppLog.Write("DeleteService Failed in UninstallPhyMemCaptureDriver", WSLMSG_ERROR);
	}

    INC_SYS_CALL_COUNT(CloseServiceHandle); 
    if(!CloseServiceHandle(Service))
	{
		AppLog.Write("ControlServiceHandle Failed in UninstallPhyMemCaptureDriver", WSLMSG_ERROR);	
	}

    INC_SYS_CALL_COUNT(CloseServiceHandle); 
	CloseServiceHandle(ServiceManager);
    return 1;
}

//Return 1 for success, 2 for any start related errors and 4 for service being already active -- RJM
ULONG InstallPhyMemCaptureDriver(void)
{
	SC_HANDLE ServiceManager=NULL, Service=NULL;
	LPTSTR FilePart=NULL;
	ULONG retFromUninst=0;

	memset(g_tempBuf,0,sizeof(g_tempBuf));

    //Uninstall driver/service.
    retFromUninst=UninstallPhyMemCaptureDriver();

	//Failure while unloading the driver
	if(retFromUninst == 3)
		return retFromUninst;

    // Open services manager.
    INC_SYS_CALL_COUNT(OpenSCManager); 
    ServiceManager = OpenSCManager(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    if (!ServiceManager)
    {
        AppLog.Write("Failed to install Physical Memory capture driver (Service Manager)", WSLMSG_ERROR);
        return 2;
    }
    
    INC_SYS_CALL_COUNT(GetFullPathName); 
    GetFullPathName(L"..\\..\\..\\Program\\vista_memcap.sys", MAX_PATH - 1, g_tempBuf, &FilePart);
	AppLog.Write(g_tempBuf,WSLMSG_STATUS);

    // Register the service.
	SetLastError(0);
    INC_SYS_CALL_COUNT(CreateService); 
	Service = NULL;
    Service = CreateService(ServiceManager,
                            L"vista_memcap",
                            L"vista_memcap",
                            SERVICE_ALL_ACCESS,
                            SERVICE_KERNEL_DRIVER,
                            SERVICE_DEMAND_START,
                            SERVICE_ERROR_NORMAL,
                            g_tempBuf,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
						    NULL);

	//If the service might already exist.

	//Open it and get its handle.
    if (GetLastError() == ERROR_SERVICE_EXISTS)
    {
        INC_SYS_CALL_COUNT(OpenService); 
        Service = OpenService(ServiceManager, L"vista_memcap", SERVICE_ALL_ACCESS);
    }
	else
    {
		AppLog.Write("Service does not exist start it!", WSLMSG_STATUS);
    }

    INC_SYS_CALL_COUNT(CloseServiceHandle); 
    CloseServiceHandle(ServiceManager);

    if (!Service)
	{
		AppLog.Write("Failed to install Physical Memory capture driver (No Service)", WSLMSG_ERROR);
		return 2;
	}

	SetLastError(0);
    // We try to run the driver/service.
    INC_SYS_CALL_COUNT(StartService); 
    if (!StartService(Service, 0, NULL))
    {
        // If already running.
        if (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
        {
			AppLog.Write("Service already active!", WSLMSG_STATUS);
            return 4;
        }
    }
    // Close handle.
    INC_SYS_CALL_COUNT(CloseServiceHandle); 
    CloseServiceHandle(Service);
    return 1;
}

//Monitor process completion using threads. --RJM 

DWORD WINAPI MonitorPComplete(LPVOID arg)
{
    WSTRemote *pRemote;

    pRemote = (WSTRemote *)arg;

    if (pRemote)
    {
        return pRemote->MonitorPComplete();
    }
    else
    {
        return -1;
    }
}

DWORD WSTRemote::MonitorPComplete()
{
	HANDLE pEvent=INVALID_HANDLE_VALUE;
	DWORD pc=0;

	//Monitor pcEvent
    INC_SYS_CALL_COUNT(OpenEvent); 
	pEvent = OpenEvent(EVENT_ALL_ACCESS,FALSE,_T("Global\\pcEvent"));

	if(pEvent == NULL)
    {
		AppLog.Write(" Unable to open status event.", WSLMSG_ERROR);
    }
    else
    {
	    while(percentComplete < 100)
	    {
            INC_SYS_CALL_COUNT(WaitForSingleObject); 
		    if(WaitForSingleObject(pEvent,INFINITE)==WAIT_OBJECT_0) // Will be generated in the driver
		    {
			    percentComplete += inputStruct.PERCENT;
			    //cout << percentComplete <<endl;
		    }	
	    }
    }
	return NULL;
}


int WSTRemote::VistaPhyMemCapture()
{
	ULONG IoControlCode=0;
	ULONG Level = 0;
	ULONG Type = 0;
	ULONG StartTime=0, EndTime=0;
	ULONG StatusCode = 0;
	ULONG BytesReturned=0;
    HANDLE hDevice; // Handle to the physical memory capture device
    WCHAR currentDirectoryPath[MAX_PATH];		
	WCHAR NtFullPathName[MAX_PATH];
    int iResult;
		
	IoControlCode = IOCTL_WRITE_RAW_DUMP;
	IoControlCode |= ((Level << 6) | (Type << 4)) & 0xF0;
	SetLastError(0);

    AppLog.Write("Installing vista_memcap driver for Vista memory capture", WSLMSG_STATUS);
	iResult = InstallPhyMemCaptureDriver();
	//Failure either in loading or unloading driver
	if(iResult == 2 || iResult == 3)
    {
        AppLog.Write("Vista memory capture driver could not be installed.", WSLMSG_STATUS);
        AppLog.Write("Physical memory capture could not be completed.", WSLMSG_STATUS);
		return 0;
    }
	
    iResult = 0;
    INC_SYS_CALL_COUNT(CreateFile); 
	hDevice = CreateFile(_T("\\\\.\\vista_memcap"), 
			        GENERIC_ALL, 
				    FILE_SHARE_READ | FILE_SHARE_WRITE, 
					NULL, OPEN_EXISTING, 
					FILE_ATTRIBUTE_NORMAL, 
					NULL);
    
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		AppLog.Write("Unable to open the memory capture driver.", WSLMSG_ERROR);
		iResult = 0;
	}
    else
    {
        // start the timer 
		INC_SYS_CALL_COUNT(GetTickCount); // Needed to be INC TKH
	    StartTime = GetTickCount();

	    memset(currentDirectoryPath,0,MAX_PATH);
	    memset(NtFullPathName,0,MAX_PATH);
        INC_SYS_CALL_COUNT(GetCurrentDirectoryW); 
	    GetCurrentDirectoryW(MAX_PATH,currentDirectoryPath);
		wcscat_s(currentDirectoryPath,L"\\TriageResults");
	
	    StringCbPrintfW(NtFullPathName, sizeof(NtFullPathName), L"\\??\\%s\\physmem.bin",currentDirectoryPath);
	    SetLastError(0);

        INC_SYS_CALL_COUNT(CreateEvent); 
	    inputStruct.pEvent = CreateEvent(NULL,FALSE,FALSE,_T("Global\\pcEvent"));
	    if (inputStruct.pEvent == NULL)
	    {
		    AppLog.Write("Could not create the event for percent complete",WSLMSG_ERROR);
	    }
	    else
	    {
//		    AppLog.Write("Created the event for percent complete",WSLMSG_STATUS);

	    }

        INC_SYS_CALL_COUNT(CreateFileW); 
	    inputStruct.phymemFile = CreateFileW(NtFullPathName,
										    FILE_APPEND_DATA|FILE_WRITE_ACCESS|SYNCHRONIZE,
										    0,
										    NULL,
										    CREATE_NEW,
										    FILE_ATTRIBUTE_NORMAL,
										    NULL);
    	
	    if(inputStruct.phymemFile == INVALID_HANDLE_VALUE)
	    {
		    AppLog.Write("Could not create the physical memory capture file",WSLMSG_ERROR);
	    }
	    else
	    {
		    phymemFile = inputStruct.phymemFile;

	        inputStruct.PERCENT=10; //Default - Tunable
	        inputStruct.SPEEDUP=128;//Default - Tunable

	        //Spawn a new thread to monitor percent complete.
	        DWORD ThreadId = 0;
            if (inputStruct.pEvent)
            {
                INC_SYS_CALL_COUNT(CreateThread); 
                if(!CreateThread(NULL,0,::MonitorPComplete,(void *)this,0,&ThreadId))
	            {
		            AppLog.Write("Could not create percent complete thread\n",WSLMSG_ERROR);
	            }
	            else
	            {
//		            AppLog.Write("New thread to monitor percentage completeion created",WSLMSG_STATUS);
	            }
            }        			

            INC_SYS_CALL_COUNT(DeviceIoControl); 
	        if (!DeviceIoControl(hDevice, IoControlCode,&inputStruct,sizeof(struct inputStruct),&hashStruct, sizeof(struct hashStruct), &BytesReturned, NULL))
	        {
		        AppLog.Write("DeviceIoControl(), Cannot send IOCTL in Vista Mem capture", WSLMSG_ERROR);
	        }
            else
            {
                // stop the timer and report the time taken
                INC_SYS_CALL_COUNT(GetTickCount); 
	            EndTime = GetTickCount();
	            _stprintf_s(g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), _T("Time taken for memory capture on Vista: %d minutes"),(EndTime -StartTime)/(60*1000));

                // copy  the hash value from the dump
	            for(int i=0;i<20;i++)
                {
		            m_memoryHash[i]=hashStruct.hash[i];
                }
	            AppLog.Write(g_tempBuf, WSLMSG_STATUS);
	            if (hashStruct.StatusCode == STATUS_SUCCESS)
                {
                    iResult = 1;
		            AppLog.Write("Physical Mem capture success.", WSLMSG_STATUS);
                }

	            UninstallPhyMemCaptureDriver(); // Unload the driver after we are done
            }
        }
        INC_SYS_CALL_COUNT(CloseHandle); 
	    CloseHandle(hDevice);
    }

    if (!iResult)
    {
        AppLog.Write("Physical Memory capture could not be completed.", WSLMSG_STATUS);
    }
	return iResult;
}
*///32 bit arch change
int Exec_PhySicalMemCapture(TCHAR *pFileexe, TCHAR *pRawCaptureFileName)
{
	if (pFileexe == NULL || pRawCaptureFileName == NULL)
	{
		return (-1);
	}

	gui.DisplayStatus(pFileexe);
	gui.DisplayStatus(pRawCaptureFileName);

	SHELLEXECUTEINFO shExInfo = { 0 };
	shExInfo.cbSize = sizeof(shExInfo); 
	shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	shExInfo.hwnd = 0;
	shExInfo.lpVerb = _T("runas"); // Operation to perform
	// Application to start
	shExInfo.lpFile = pFileexe; //Driver absolute Path 
	shExInfo.lpParameters = pRawCaptureFileName; // raw file abs path // Additional parameters
	shExInfo.lpDirectory = 0;
	shExInfo.nShow = SW_HIDE;
	shExInfo.hInstApp = 0;

	if (ShellExecuteEx(&shExInfo))
	{
		WaitForSingleObject(shExInfo.hProcess, INFINITE);
		CloseHandle(shExInfo.hProcess);
		gui.DisplayStatus(_T("Physical Memory Capture Successfully Completed.."));	
	}
	else 
	{
		gui.DisplayStatus(_T("Physical Memory Capture failed.."));
	}	
	
	return 0;

}

void WSTRemote::CapturePhysicalMemory()
{
#ifdef USE_WPMEM
    TCHAR driverpath[256];
    TCHAR rawfilename[256];
	
	AppLog.Write("WSTRemote::CapturePhysicalMemory()", WSLMSG_DEBUG);
	AppLog.Write("Capturing physical system memory.", WSLMSG_STATUS);
	_tcscpy(driverpath, uslattDrive);
	_tcscat(driverpath, _T("\\Program"));
	_tcscpy(rawfilename,ca_dirName);
	_tcscat(rawfilename, _T("\\TriageResults\\physicalmem.raw"));	
	    
	if(Is64BitWindows( )) 
	{
	    _tcscat(driverpath,_T("\\\\winpmem_mini_x64_rc2.exe"));
		Exec_PhySicalMemCapture(driverpath,rawfilename);
	}
	else 
	{
		_tcscat(driverpath,_T("\\\\winpmem_mini_x86.exe"));
		Exec_PhySicalMemCapture(driverpath,rawfilename);
	}
	
#else 
	AppLog.Write("WSTRemote::CapturePhysicalMemory()", WSLMSG_DEBUG);

	SysData sd;
	tstring filename;
	int retFromVistaMemCap=0;


	AppLog.Write("Capturing physical system memory.", WSLMSG_STATUS);
	percentComplete=0;


	SYSTEM_INFO si;
	INC_SYS_CALL_COUNT(GetSystemInfo); // Needed to be INC TKH
	GetSystemInfo(&si);

	if((g_osvex.dwMajorVersion >= 6 )&& (si.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_AMD64 && 
		si.wProcessorArchitecture!= PROCESSOR_ARCHITECTURE_IA64 ))
	{
		AppLog.Write("** Windows version 6.0 or higher (Vista/Windows 7/2008) detected.", WSLMSG_STATUS);

		BOOL f64=FALSE;
		INC_SYS_CALL_COUNT(IsWow64Process); // Needed to be INC TKH
		INC_SYS_CALL_COUNT(GetCurrentProcess); // Needed to be INC TKH
		IsWow64Process(GetCurrentProcess(),&f64);
		//x64code
		if(f64)
		{
			ULONG StartTime=0;
			ULONG EndTime=0;
			INC_SYS_CALL_COUNT(GetTickCount); 
			StartTime = GetTickCount();
			DWORD retFromDump = DumpMemory64(1);//32 bit arch change
			if(retFromDump)
			{
				memset(g_tempBuf,0,MAX_PATH);
				_sntprintf_s(g_tempBuf,MAX_PATH,MAX_PATH,L"x64 windows memory collection failed %d",retFromDump);
				AppLog.Write(g_tempBuf,WSLMSG_ERROR);
			}
			else
			{
				INC_SYS_CALL_COUNT(GetTickCount); 
				EndTime = GetTickCount();
				_stprintf_s(g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), _T("Time taken for 64bit memory capture: %d minutes"),(EndTime -StartTime)/(60*1000));
				AppLog.Write(g_tempBuf,WSLMSG_STATUS);
			}
		}
		else
		{
			//32 bit arch change
			/*
			AppLog.Write("Loading x86  windows driver for memory collection ",WSLMSG_STATUS);
			percentComplete = 0; // Write to pipe that mem capture beginning
			
			retFromVistaMemCap = VistaPhyMemCapture(); 
			//percentComplete = 100; // Write to pipe that mem capture ended 

			if(retFromVistaMemCap == 1)
			{
				AppLog.Write("Physical memory capture completed successfully.", WSLMSG_STATUS);
				m_bHaveMemory = true;
				//Write the hash to the digest file
				tstring hash;
				hash=m_hasher.MD5BytesToString(m_memoryHash);
				WriteHashes(_T("\\TriageResults\\physmem.bin"),hash);
			}
			*/
			//32 bit arch change
			ULONG StartTime=0;
			ULONG EndTime=0;
			INC_SYS_CALL_COUNT(GetTickCount); 
			StartTime = GetTickCount();
			DWORD retFromDump = DumpMemory64(0);
			if(retFromDump)
			{
				memset(g_tempBuf,0,MAX_PATH);
				_sntprintf_s(g_tempBuf,MAX_PATH,MAX_PATH,L"x32 windows memory collection failed %d",retFromDump);
				AppLog.Write(g_tempBuf,WSLMSG_ERROR);
			}
			else
			{
				INC_SYS_CALL_COUNT(GetTickCount); 
				EndTime = GetTickCount();
				_stprintf_s(g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), _T("Time taken for 32bit memory capture: %d minutes"),(EndTime -StartTime)/(60*1000));
				AppLog.Write(g_tempBuf,WSLMSG_STATUS);
			}
		}
	}
	//x64code
	/*else if((g_osvex.dwMajorVersion >= 6) && (si.wProcessorArchitecture!=PROCESSOR_ARCHITECTURE_AMD64 || 
											 si.wProcessorArchitecture!= PROCESSOR_ARCHITECTURE_IA64 ))
	{
		AppLog.Write("Vista Memory Capture on 64 bit machines not supported", WSLMSG_ERROR);	
		AppLog.Write("Memory could not be captured.", WSLMSG_STATUS);	
	}	*/
    else
    {
        filename = TEXT("TriageResults\\physmem.bin");
		//Third parameter is compression-level which is set to 0
	    if (sd.GetPhysicalMemory(0, filename,0, percentComplete, m_memoryHash) == SysData::SUCCESS) 
	    {
		    _stprintf_s(g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), _T("Physical memory capture successfully written to file: %s"), filename.c_str());
		    AppLog.Write(g_tempBuf, WSLMSG_STATUS);
			AppLog.Write("Physical memory capture completed successfully.", WSLMSG_STATUS);
            m_bHaveMemory = true;
			//Write the hash to the digest file
			tstring hash;
			hash=m_hasher.MD5BytesToString(m_memoryHash);
			WriteHashes(_T("\\TriageResults\\physmem.bin"),hash);
	    }
	    else
        {
		    AppLog.Write("There has been an error capturing the physical memory.", WSLMSG_STATUS);
		    AppLog.Write("Memory could not be captured.", WSLMSG_STATUS);	
        }	
    }
#endif

}

void WSTRemote::GetEventLogPaths(HashedXMLFile &repository)
{	
	EventLogParser elp;

	AppLog.Write("WSTRemote::GetEventLogPaths()", WSLMSG_DEBUG);
	AppLog.Write("Parsing System Event Logs", WSLMSG_STATUS);

	if(elp.Start(repository))
	{
		AppLog.Write("Done collecting event logs.", WSLMSG_STATUS);
	}
	else
	{
		AppLog.Write("There has been an error parsing the system event logs.\n", WSLMSG_STATUS);
	}
}

#ifndef NETFORENSICS
// Old not needed anymore
void WSTRemote::GetNetworkInfo(HashedXMLFile &repository)
{
	NetworkInfo ni;

	AppLog.Write("Collecting network information.", WSLMSG_STATUS);

	AppLog.Write("Enumerating network adapters.", WSLMSG_STATUS);
	ni.EnumerateAdapters();

	if(config.getNetStat)
	{
		AppLog.Write("Retrieving open network ports.", WSLMSG_STATUS);
        
		ni.GetPortsAndPIDS();
	}
	

	ni.WriteXML(repository);


	AppLog.Write("Done collecting network information.", WSLMSG_STATUS);
}
#endif

void AddStructToXML(HashedXMLFile &repository, int i, LPNETRESOURCE lpnrLocal)
{
	static int count=0;
 

    repository.OpenNodeWithAttributes("NetworkDrive");
	_itot_s(count, g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), 10);
    repository.WriteAttribute("DriveNum", g_tempBuf);
    repository.Write(">");
	
    switch (lpnrLocal->dwType) {
    case (RESOURCETYPE_ANY):
		_tcsncpy_s(g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), _T("any"),4);
        break;
    case (RESOURCETYPE_DISK):
        _tcsncpy_s(g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), _T("disk"),5);
        break;
    case (RESOURCETYPE_PRINT):
        _tcsncpy_s(g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR), _T("print"),6);
        break;
    default:
        _tcsncpy_s(g_tempBuf, sizeof(g_tempBuf)/sizeof(TCHAR),_T("unknown"),8);
        break;
    }
    repository.WriteNodeWithValue("ResourceType", g_tempBuf);
  
    if (lpnrLocal->lpLocalName)
    {
        repository.WriteNodeWithValue("Localname", lpnrLocal->lpLocalName);
	    unsigned _int64 totBytes;
		//network.push_back(tstring(lpnrLocal->lpLocalName)+_T("\\"));
        INC_SYS_CALL_COUNT(GetDiskFreeSpaceEx); 
		
	    GetDiskFreeSpaceEx(lpnrLocal->lpLocalName,NULL,NULL,(PULARGE_INTEGER) &totBytes);
    }
    if (lpnrLocal->lpRemoteName)
    {
        repository.WriteNodeWithValue("Remotename", lpnrLocal->lpRemoteName);
    }
	count++;
    repository.CloseNode("NetworkDrive");
	
	
}


//***************************************************************************
//  Function looks for drives that might be available only while the system

//  is still in the live state.
//***************************************************************************

//void WSTRemote::GetTrueCryptDriveInfo(map<char,char *> &driveMap)
bool WSTRemote::IsTrueCryptDrive(char driveLet, wstring &szVolFile)
{
    DWORD dwResult;
    HANDLE hDriver;
    BOOL bResult;
    bool bRetVal = false;
    TCHAR cDrive[3]=_T("");

    INC_SYS_CALL_COUNT(CreateFile); 
    hDriver = CreateFile (_T("\\\\.\\TrueCrypt"), 0, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (hDriver != INVALID_HANDLE_VALUE)
    {
		LONG DriverVersion;

        // TruCrypt Drive installed and opened! 
		// No let's check the version and interogate.
        INC_SYS_CALL_COUNT(DeviceIoControl); 
		bResult = DeviceIoControl(hDriver,TC_IOCTL_GET_DRIVER_VERSION, NULL,0,&DriverVersion,sizeof (DriverVersion), &dwResult, NULL);
        if (!bResult)
        {
            INC_SYS_CALL_COUNT(DeviceIoControl); 
			bResult = DeviceIoControl(hDriver, TC_IOCTL_LEGACY_GET_DRIVER_VERSION,0,NULL,&DriverVersion, sizeof(DriverVersion),&dwResult,NULL);
        }
		
        if (bResult)
        {
            MOUNT_LIST_STRUCT mlist;
		    int i; 
		    int driveCount=0;
		    wstring volume;
		    memset (&mlist, 0, sizeof (mlist));
		    int count=0;

            if (DriverVersion >= 0x500 && DriverVersion <= 0x71a)//Newest Version is 71a (keep monitoring this on the site)
            {
                INC_SYS_CALL_COUNT(DeviceIoControl); 
			    if (DeviceIoControl (hDriver, TC_IOCTL_GET_MOUNTED_VOLUMES, &mlist,sizeof (mlist), &mlist, sizeof (mlist), &dwResult,NULL))
                {
                    INC_SYS_CALL_COUNT(CloseHandle); 
                    CloseHandle(hDriver);
				    i = tolower(driveLet)-'a';
				    if (wcslen((const wchar_t *)mlist.wszVolume[i]))
				    {
                        szVolFile = reinterpret_cast<wchar_t *>(mlist.wszVolume[i]);
                        return true;
				    }
				    else
                    {
					    return false;
                    }
			    }
            }
            else if (DriverVersion < 0x500)
            {
                INC_SYS_CALL_COUNT(DeviceIoControl); 
			    if (DeviceIoControl (hDriver, TC_IOCTL_LEGACY_GET_MOUNTED_VOLUMES, &mlist,sizeof (mlist), &mlist, sizeof (mlist), &dwResult,NULL))
                {
                    INC_SYS_CALL_COUNT(CloseHandle); 
 		            CloseHandle(hDriver);
				    i = tolower(driveLet)-'a';
				    if (wcslen((const wchar_t *)mlist.wszVolume[i]))
				    {
                        szVolFile = reinterpret_cast<wchar_t *>(mlist.wszVolume[i]);
                        return true;
				    }
				    else
                    {
					    return false;
                    }
                }
            }
        }
        INC_SYS_CALL_COUNT(CloseHandle); 
		CloseHandle(hDriver);
	}
	wstring trueCryptFailed (L"NO_TRUE_CRYPT");
	//trueCryptFailed.;
	return false;
}

//***************************************************************************
//  Function looks for drives that might be available only while the system

//  is still in the live state.
//***************************************************************************
bool WSTRemote::IsFreeOTFEDrive(char driveL, wstring &wszPath)
{
    DWORD dwResult;
    HANDLE hDriver;
    int iResult;
	wchar_t deviceNameW[MAX_PATH]=L"";
    TCHAR szDriveLetter[3];
    bool bRetVal = false;

    INC_SYS_CALL_COUNT(CreateFile); 
    hDriver = CreateFile (_T("\\\\.\\FreeOTFE\\FreeOTFE"), GENERIC_READ, 1, NULL, OPEN_EXISTING, 0, NULL);
	
    if (hDriver != INVALID_HANDLE_VALUE)
    {
		AppLog.Write("Located the FreeOTFE Driver.", WSLMSG_STATUS);
		DIOC_VERSION DriverVersion;
		DIOC_DEVICE_NAME_LIST driveList;

        INC_SYS_CALL_COUNT(DeviceIoControl); 
		iResult = DeviceIoControl(hDriver,IOCTL_FREEOTFE_VERSION, NULL,0,&DriverVersion,sizeof(DriverVersion), &dwResult, NULL);

		if(!iResult)
		{
            INC_SYS_CALL_COUNT(CloseHandle); 
            CloseHandle(hDriver);
			AppLog.Write("DeviceIoControl call failed in IsFreeOTFEDrive", WSLMSG_STATUS);
			wszPath = L"DVIO_FAILED_FREEOTFE";
        }
        else
        {
            INC_SYS_CALL_COUNT(DeviceIoControl); 
		    iResult = DeviceIoControl(hDriver,IOCTL_FREEOTFE_GET_DISK_DEVICE_LIST,NULL,0,&driveList,sizeof(driveList), &dwResult, NULL);
		    if(!iResult)
		    {
			    AppLog.Write("DeviceIoControl call failed in IsFreeOTFEDrive", WSLMSG_STATUS);
			    wszPath = L"DVIO_FAILED_FREEOTFE";
		    }
            else
            {

		        AppLog.Write("Device I/O Control call returned.", WSLMSG_STATUS);

				wchar_t driveW[4];
				mbstowcs(driveW,&driveL,_TRUNCATE);
                szDriveLetter[0]=driveW[0];
		        szDriveLetter[1]=_T(':');
		        szDriveLetter[2]=_T('\0');

				
		        memset(g_tempBuf,0,sizeof(g_tempBuf));
                INC_SYS_CALL_COUNT(QueryDosDevice); 
		        int ret = QueryDosDevice(szDriveLetter,g_tempBuf,sizeof(g_tempBuf));//CRASHCHECK

		        for(int i=0;i<27 && !bRetVal;i++)
		        {	
			        if(!_tcscmp(g_tempBuf,driveList.DeviceName[i]))
			        {
				       
						wszPath=g_tempBuf;
				        bRetVal = true;
			        }
		        }
            }
        }

        INC_SYS_CALL_COUNT(CloseHandle); 
		CloseHandle(hDriver);
	}
	else
	{
		wszPath = L"NO_FREEOTFE";
	}

    return bRetVal;
}


void WSTRemote::GetInstalledAppInfo(HashedXMLFile &repository)
{
	HKEY hkPath;
	HKEY hkKey;
	unsigned int iIndex = 0;
	unsigned int iNumKeys = 0;
	unsigned int iMaxKeyLen = 0;
	unsigned int iKeyLen = 0;
	unsigned int iValueLen = 0;
	LONG lResult = 0;
	LPTSTR pKeyName = NULL;
	LPTSTR pAppName = NULL;


	AppLog.Write("Identifying installed applications.", WSLMSG_STATUS);


    INC_SYS_CALL_COUNT(RegOpenKeyEx); 
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"), 0, KEY_READ, &hkPath) == ERROR_SUCCESS)
	{
        INC_SYS_CALL_COUNT(RegQueryInfoKey); 
        if (RegQueryInfoKey(hkPath, NULL, NULL, NULL, (DWORD*)&iNumKeys, (DWORD*)&iMaxKeyLen, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            repository.OpenNode("Applications");

	        iKeyLen = iMaxKeyLen + 2;
	        pKeyName = (LPTSTR)malloc(iKeyLen * sizeof(TCHAR));//CRASHCHECK

            INC_SYS_CALL_COUNT(RegEnumKeyEx); 
	        while((lResult = RegEnumKeyEx(hkPath, iIndex, pKeyName, (DWORD*)&iKeyLen, NULL, NULL, NULL, NULL)) != ERROR_NO_MORE_ITEMS)
	        {
		        // Skip this entry as there's no app name
                INC_SYS_CALL_COUNT(RegOpenKeyEx); 
		        if(RegOpenKeyEx(hkPath, pKeyName, NULL, KEY_READ, &hkKey) == ERROR_SUCCESS)
		        {
                    repository.OpenNode("App");
		            // Here's to hoping none of these are more than MAX_PATH long
		            iValueLen = MAX_PATH;
                    INC_SYS_CALL_COUNT(RegQueryValueEx); 
		            if (RegQueryValueEx(hkKey, _T("DisplayName"), NULL, NULL, (LPBYTE)g_tempBuf, (DWORD*)&iValueLen) == ERROR_SUCCESS)
                    {
                        repository.WriteNodeWithValue("Name", g_tempBuf);
                    }

		            iValueLen = MAX_PATH;
                    INC_SYS_CALL_COUNT(RegQueryValueEx); 
		            if (RegQueryValueEx(hkKey, _T("DisplayVersion"), NULL, NULL, (LPBYTE)g_tempBuf, (DWORD*)&iValueLen) == ERROR_SUCCESS)
                    {
                        repository.WriteNodeWithValue("Version", g_tempBuf);
                    }
	
	                iValueLen = MAX_PATH;		
                    INC_SYS_CALL_COUNT(RegQueryValueEx); 
	                if (RegQueryValueEx(hkKey, _T("InstallLocation"), NULL, NULL, (LPBYTE)g_tempBuf, (DWORD*)&iValueLen) == ERROR_SUCCESS)
                    {
                        repository.WriteNodeWithValue("Path", g_tempBuf);
                    }

		            iValueLen = MAX_PATH;		
                    INC_SYS_CALL_COUNT(RegQueryValueEx); 
		            if (RegQueryValueEx(hkKey, _T("Publisher"), NULL, NULL, (LPBYTE)g_tempBuf, (DWORD*)&iValueLen) == ERROR_SUCCESS)
                    {
                        repository.WriteNodeWithValue("Publisher", g_tempBuf);
                    }

                    repository.CloseNode("App");
                }
		        memset(pKeyName, 0, iMaxKeyLen * sizeof(TCHAR));//CRASHCHECK
		        iKeyLen = iMaxKeyLen + 2;
		        iIndex++;
	        }
            repository.CloseNode("Applications");
	        free(pKeyName);
        }
        INC_SYS_CALL_COUNT(RegCloseKey); 
        RegCloseKey(hkPath);
    }    
	AppLog.Write("Application identification completed.", WSLMSG_STATUS);
}

//***************************************************************************************
//
// GetDriverInfo
//
//***************************************************************************************
void WSTRemote::GetDriverInfo(HashedXMLFile &repository)
{
    repository.OpenNodeWithAttributes("Drivers");
}

//-->RJM Microsoft Knowledge base -- 131065


extern BOOL SetPrivilege( 
	HANDLE hToken,  // token handle 
	LPCTSTR Privilege,  // Privilege to enable/disable 
	BOOL bEnablePrivilege  // TRUE to enable. FALSE to disable 
);
//***************************************************************************************
//
// GetRegistry
//
//***************************************************************************************
void WSTRemote::GetRegistry(HashedXMLFile &repository,vector<tstring> drives,map<tstring,int> extensions)
{
	
	AppLog.Write("WSTRemote::GetRegistry()", WSLMSG_DEBUG);
	DWORD dwRegs = 0;

	AppLog.Write(_T("Parsing system registry to repository"), WSLMSG_STATUS);
	repository.OpenNode("Registry"); 
	
	//if(config.getHKROOT)
	{
		dwRegs = dwRegs | GET_HKCR;
		//AppLog.Write(_T("Collecting HKEY_CLASSES_ROOT"), WSLMSG_STATUS);
	}
	//if(config.getHKLM)
	{
		dwRegs = dwRegs | GET_HKLM;
		//AppLog.Write(_T("Collecting HKEY_LOCAL_MACHINE"), WSLMSG_STATUS);
	}
	//if(config.getHKUSERS)
	{
		dwRegs = dwRegs | GET_HKU;
		//AppLog.Write(_T("Collecting HKEY_USERS"), WSLMSG_STATUS);
	}
	//if(config.getHKCONFIG)
	{
		dwRegs = dwRegs | GET_HKCC;
		//AppLog.Write(_T("Collecting HKEY_CURRENT_CONFIG"), WSLMSG_STATUS);
	}
	

	

	//EnumerateRegistry(dwRegs, repository);

	map<wstring,int >::iterator eit;
	vector<wstring> extensionList;
	for(eit=(extensions).begin();eit!=(extensions).end();eit++)
	{
			extensionList.push_back((*eit).first);
	}

	ExtractRegistryInfo(repository,drives,extensionList,config.getMaximumFileSize);

	//AppLog.Write(_T("Completed collecting MRU information about files"), WSLMSG_STATUS);

	//LPCTSTR inputKey="Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU";
	map<tstring,HKEY> HKEY_MAP;

	HKEY_MAP[_T("HKEY_LOCAL_MACHINE")]=HKEY_LOCAL_MACHINE;
	HKEY_MAP[_T("HKEY_CLASSES_ROOT")]=HKEY_CLASSES_ROOT;
	HKEY_MAP[_T("HKEY_CURRENT_USER")]=HKEY_CURRENT_USER;
	HKEY_MAP[_T("HKEY_CURRENT_CONFIG")]=HKEY_CURRENT_CONFIG;
	
	/*
	vector<tstring>::iterator it;
	for(it=config.MapRegistryKey.begin();it<config.MapRegistryKey.end();it++)
	{
		HANDLE hToken=INVALID_HANDLE_VALUE;
		HANDLE hCurrentThread=INVALID_HANDLE_VALUE;

		INC_SYS_CALL_COUNT(GetCurrentThread);
		hCurrentThread = GetCurrentThread();
		INC_SYS_CALL_COUNT(OpenThreadToken);
		if(!OpenThreadToken(hCurrentThread, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
		{
			if (GetLastError() == ERROR_NO_TOKEN)
			{
				INC_SYS_CALL_COUNT(ImpersonateSelf);
				if (!ImpersonateSelf(SecurityImpersonation))
					;
//					rv = WINAPI_FAILURE;

				INC_SYS_CALL_COUNT(OpenThreadToken);
				INC_SYS_CALL_COUNT(GetCurrentThread); // Needed to be INC TKH
				if(!OpenThreadToken(GetCurrentThread(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, FALSE, &hToken))
					;
//					rv = WINAPI_FAILURE;
			}	
		}
		// enable SeDebugPrivilege
		INC_SYS_CALL_COUNT(SetPrivilege);
		if(!SetPrivilege(hToken, SE_TAKE_OWNERSHIP_NAME, TRUE))
		{
			// close token handle
			AppLog.Write("Could not elevate token privilege level",WSLMSG_ERROR);
			INC_SYS_CALL_COUNT(CloseHandle);
			CloseHandle(hToken);
		} 
		//LPCTSTR inputKey="Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32";
		int split0 = (*it).find(_T("\\"));
		tstring hive=(*it).substr(0,split0);
		tstring keyStr=(*it).substr(split0+2);
		int split1=(keyStr).find(_T('|'));
		tstring inputKey=(keyStr).substr(0,split1);
		tstring rest=(keyStr).substr(split1+1);
		int split2=rest.find(_T('|'));
		int captureFlag=_tatoi(const_cast<TCHAR *>(rest.substr(0,split2).c_str()));
		tstring displayName=(rest.substr(split2+1));
		
		
		SetLastError(0);
		HKEY hKey;
		DWORD status=0; 
		//HKEY cuKey;
		//LPCTSTR inputKey="Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32";
		//cout<<HKEY_MAP[hive]<<" "<<inputKey<<endl;
		LPTSTR key=const_cast<TCHAR *>(inputKey.c_str());
		INC_SYS_CALL_COUNT(RegOpenKeyEx); // Needed to be INC TKH
		status=RegOpenKeyEx(HKEY_MAP[hive],key,0,KEY_READ,&hKey); 
		if(status==2)
		{
			AppLog.Write("Trying for the second time with WRITE_OWNER",WSLMSG_STATUS);
			INC_SYS_CALL_COUNT(RegOpenKeyEx); // Needed to be INC TKH
			status=RegOpenKeyEx(HKEY_MAP[hive],key,0,WRITE_OWNER,&hKey); 
			if(status==2)
				AppLog.Write("Failed for the second time due to lack of privilege",WSLMSG_STATUS);

		}

		if(status == ERROR_SUCCESS)
		{
			LPCTSTR name= displayName.c_str();
			TraverseRegistry(hKey,name,captureFlag,repository);
			tstring msg=_T("Captured registry key ")+hive+_T("\\\\")+tstring(inputKey);
			AppLog.Write(msg.c_str(),WSLMSG_STATUS);
		}
		else
		{
			tstring msg=_T("Could not capture registry key ")+hive+_T("\\\\")+tstring(inputKey);
			AppLog.Write(msg.c_str(),WSLMSG_ERROR);		
		}
		if(hKey)
		{
			INC_SYS_CALL_COUNT(RegCloseKey); // Needed to be INC TKH
			RegCloseKey(hKey);
		}
		
	}
	*/

		repository.CloseNode("Registry");
    AppLog.Write(_T("Completed Registry collection"), WSLMSG_STATUS);

	
}

//This function compresses data from an input file to a gzip format based on the 
//compress flag. If the compress flag is 0 then there is no compression. --RJM

int WSTRemote::compressData(const char *ipzFileName,const char *opzFileName,int compressFlag)
{
    return 1;
}



/* ================================================
			!!!!!!!!!WARNING!!!!!!!!!
	The follong function provides IN-PLACE encryption
	for a given filename.  IT IS DESTRUCTIVE TO THE
	ORIGINAL FILE!!!  Keep that in mind before you
	decide to use/modify it!
================================================ */
int WSTRemote::Encrypt(const char *pzFileName)
{
	return 1;
}


#undef _UNICODE
#undef UNICODE
