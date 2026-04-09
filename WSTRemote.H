#pragma once

#define _UNICODE
#define UNICODE 

#include <fcntl.h>
#include <io.h>
//#include <vector>
#include "stdafx.h"
#include "FileHasher.h"
#include "SysData.h"
#include "xmlParser.h"
#include "PipeCommHandler.h"
#include "defs.h"
#include "HashedXML.h"
#include <map>
#include <string>
#include <stdexcept>
#include "tstl.h"
#include "gui.h" // For new integrated GUI
#include "WSTLog.h"
#include "WSTCommon.h"

using namespace std;

//extern tstring globalCaseDir;

#define MEM_LIMIT 10485760 //10MB
#define MINIMUM_SPACE_REQUIRED 5*1024*1024 //5MB (minimum amount of space required for a triage)
//For State Aware insertion


#pragma comment(lib, "netapi32.lib")

#define CONFIG_ROOT_NAME _T("TARGET_SCAN_CONFIG")
#define NO_SCREEN_CAPTURE -2//RJM
#define ABORT_FROM_SCREENSHOT -3 //RJM
#define WINDOWS_VERSION_ERROR -4//RJM

#define BEGIN 
#define END
// maximum file size to hash in bytes --RJM
//static unsigned long double MaxFileSize;

//TrueCrypt detection

#define TC_IOCTL(CODE) (CTL_CODE (FILE_DEVICE_UNKNOWN, 0x800 + (CODE), METHOD_BUFFERED, FILE_ANY_ACCESS))

#define TC_IOCTL_GET_DRIVER_VERSION TC_IOCTL (1)

#define TC_IOCTL_GET_MOUNTED_VOLUMES TC_IOCTL (6)

#define TC_IOCTL_LEGACY_GET_DRIVER_VERSION 466968

#define TC_IOCTL_LEGACY_GET_MOUNTED_VOLUMES 466948

//For FreeOTFE detection. 

#define FREEOTFE_MAX_FILENAME_LENGTH 1024

#define IOCTL_FREEOTFE_GET_DISK_DEVICE_LIST CTL_CODE(              \
                                                FILE_DEVICE_DISK,  \
                                                0x808,             \
                                                METHOD_BUFFERED,   \
                                                FILE_READ_DATA     \
                                               )

#define IOCTL_FREEOTFE_GET_DISK_DEVICE_COUNT CTL_CODE(             \
                                                FILE_DEVICE_DISK,  \
                                                0x807,             \
                                                METHOD_BUFFERED,   \
                                                FILE_READ_DATA     \
                                               )
#define IOCTL_FREEOTFE_VERSION         CTL_CODE(                   \
                                                FILE_DEVICE_DISK,  \
                                                0x800,             \
                                                METHOD_BUFFERED,   \
                                                FILE_READ_DATA     \
                                               )

#define IOCTL_FREEOTFE_GET_DISK_DEVICE_METADATA  CTL_CODE(         \
                                                FILE_DEVICE_DISK,  \
                                                0x804,             \
                                                METHOD_BUFFERED,   \
                                                FILE_READ_DATA     \
                                               )

typedef struct _DIOC_VERSION {
    ULONG VersionID;    
} DIOC_VERSION, *PDIOC_VERSION;

// This struct returns a list of device kernel mode names
typedef struct _DIOC_DEVICE_NAME_LIST {
    ULONG DeviceCount;
    TCHAR DeviceName[26][FREEOTFE_MAX_FILENAME_LENGTH];  // Variable number of device names
} DIOC_DEVICE_NAME_LIST, *PDIOC_DEVICE_NAME_LIST;

typedef struct _DIOC_DISK_DEVICE_METADATA {
    ULONG MetaDataLength;  // In bytes
    TCHAR MetaData[4096];
} DIOC_DISK_DEVICE_METADATA, *PDIOC_DISK_DEVICE_METADATA;

typedef struct _DIOC_DEVICE_NAME {
    TCHAR DeviceName[FREEOTFE_MAX_FILENAME_LENGTH];
} DIOC_DEVICE_NAME, *PDIOC_DEVICE_NAME;


/*-->RJM - Vista mem capture using win32dd*/
#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define FILE_DEVICE_UNKNOWN 0x00000022

#define METHOD_BUFFERED 0

#define FILE_READ_DATA ( 0x0001 )    // file & pipe
#define FILE_WRITE_DATA ( 0x0002 )   // file & pipe

#define IOCTL_WRITE_RAW_DUMP CTL_CODE(FILE_DEVICE_UNKNOWN, 0x1, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)
#define IOCTL_WRITE_CRSH_DUMP CTL_CODE(FILE_DEVICE_UNKNOWN, 0x2, METHOD_BUFFERED, FILE_READ_DATA | FILE_WRITE_DATA)

#define STATUS_SUCCESS 'ENOD'
#define STATUS_UNSUCCESSFUL 'LIAF'
/*<--RJM - Vista mem capture using win32dd*/

/*class XMLNodeNotFound : public std::runtime_error
{
public:
	XMLNodeNotFound():std::runtime_error("XMLNodeNotFound"){}
};*/

typedef struct

{
	unsigned __int32 ulMountedDrives;     /* Bitfield of all mounted drive letters */
	short wszVolume[26][MAX_PATH];        /* Volume names of mounted volumes */
	unsigned __int64 diskLength[26];
	int ea[26];
	int volumeType[26];       /* Volume type (e.g. PROP_VOL_TYPE_OUTER, PROP_VOL_TYPE_OUTER_VOL_WRITE_PREVENTED, etc.) */

} MOUNT_LIST_STRUCT;

//TrueCrypt detection

extern OSVERSIONINFOEX g_osvex;

extern int config_MAX_TAB_COUNT;
extern int config_SLEEP_SECONDS;
extern int config_MRU_TIME;

class PipeCommHandler;

//
// WSTRemote: Hashes files on a remote system while listening for requests from a main investigator machine.
//			Acquires details about the system being scanned.
//			Captures process memory, screenshots, .....
//			And much much more coming soon!
//		
class WSTRemote
{

public:		// member functions

	WSTRemote(tstring configFilename);
	~WSTRemote(void);
		
	wstring FreeOTFEDriveInfo(char driveLet);
	DWORD FileType(LPWIN32_FIND_DATA pfileData);
	BOOL TraverseDirectory(LPCTSTR pathName,map<string,int>);

	// kick off scan based on settings read in from the config file
	int DoInvestigation();

	int DoFileScan();
	int compressData(const char *,const char *,int);

	//XMLNode GatherSystemData();			// main collection function based on configuration file
        //int GetSystemData(XMLNode &root);
    int GetSystemData(HashedXMLFile &repository);

	void GetOSInfo();
	void GetHardwareInfo();
#ifndef NETFORENSICS
	void GetNetworkInfo(HashedXMLFile &repository);
	//void getWiFiInfo(HashedXMLFile &repository);
#endif
#ifdef NETFORENSICS
	int doNetForensics(WSTCommon &commonResources);
#endif
	int  startSearch(vector<tstring> drives,map<tstring,int> extensions);
	void GetInstalledAppInfo(HashedXMLFile &repository);
	void GetRecentEventInfo();
	void GetEventLogPaths(HashedXMLFile &repository);
    bool IsTrueCryptDrive(char driveLet, wstring &szVolFile);
#ifdef TRUECRYPT
	int ImageTrueCryptDrives(HashedXMLFile &repository);
	char driveToImage[3];
#endif

#ifdef DRIVE_IMAGING
	int WSTRemote::ImageSelectedExternalDrives(HashedXMLFile &repository);
#endif
#ifdef MAILFILECAPTURE
	int CaptureMail();
#endif
#ifdef WEBFORENSICS
	int ExtractWebActivity(WSTLog &AppLog);
#endif
#ifdef TIMELINE
	int ExtractStartupShutdown();
#endif
    bool IsFreeOTFEDrive(char driveL, wstring &szPath);
    void GetDiskInfo(HashedXMLFile &repository);
    void GetDriverInfo(HashedXMLFile &repository);
    void GetRegistry(HashedXMLFile &repository,vector<tstring> drives,map<tstring,int> extensions);
	void ExtractRegistryInfo(HashedXMLFile &repository,vector<tstring> drives,vector<wstring> extensionList,unsigned long long maxFileSize);
	bool TraverseRegistry(HKEY hKey,LPCTSTR displayName,bool captureSubKey,HashedXMLFile &repository,LPTSTR KeyName=NULL);
    int GetLoggedInUsers(HashedXMLFile &repository);
    bool GetDriveGeo(int iDriveNumber, HashedXMLFile &repository);
    void GetDiskVolumes(HashedXMLFile &repository);
    DWORD MonitorPComplete();
	
	void CaptureProcessMemory();
	int WSTRemote::VistaPhyMemCapture();
	void CapturePhysicalMemory();
	int CaptureScreenshot(int,int,int);
	int  SetupCaseDirectory(); //RJM
	int MoveAndHashResults();
#ifdef RECENTDOCS
	int getRecentDocs(bool getCurrentUserOnly,bool getAllUsers,bool followNw,bool followLocal,bool followRemovable,vector<tstring> extensionList);
#endif

	int DumpMemory64(int arch64);//x64Code 
								 // New change for 32 bit arch
	

private:	// member variables

	// configuration from investigator
	ScanConfig config;
	

	tstring pipeName;			// command pipe name

	FileHasher m_hasher;						// file hashing object

	
	tfstream fsHash;						// hash computations --RJM
	int supports_64_bit;						//RJM for AMD 64 bit detection.

    char m_szCurrentDrive[2];

	

	int Encrypt(const char *pzFileName);
	int iUseEncryption;
	tstring RetrievalMask;
    bool m_bHaveMemory;
    unsigned char m_repositoryHash[20];
    unsigned char m_memoryHash[20];
    tstring m_szRepositoryFile;
	TCHAR ca_dirName[MAX_PATH];
public:

	friend class PipeCommHandler;

	ScanState currentState;
	bool doneReported;
	bool finished;


	// counters
	unsigned int filesProcessed;
	unsigned int procsCaptured;
	string fileName;
	unsigned int totalFileCount;
	int percentComplete;
	tstring currentEncryptedFile;

	void SignalAbort();						// called by communication handler when abort command is received

	

#ifdef MEMMONITOR
	HANDLE LowMemLevelMutex;
	BOOLEAN bDiskFull;
#endif

	
	
private:	// member functions

	void ResetConfig();						// basically turn everything off
	bool LoadConfigFile(tstring filename);	// load configuration out of an XML file
	void OutputConfig();

	int ProcessScanPath(tstring scanPath);

	
	SYSTEMTIME beginTime, endTime;

	// hash file output stuff
	void OutputHashfileHeader();
	void OutputMatch(tstring filename, time_t time);
	void OutputMatch(tstring filename, unsigned char *md5, time_t time);
	void OutputStatus(tstring filename, tstring type);
	void OutputAttrib(
		tstring filename, 
		_fsize_t size, 
		time_t mtime, 
		time_t atime, 
		time_t ctime,
		tstring ownersid,
		tstring groupsid);
	void OutputEnd(unsigned int searched, unsigned int matched);

	int EnumerateServices(HashedXMLFile &repository, int &iServiceCount);

	
	// checks strings for invalid/unprintable characters that XML doesn't like
	tstring XMLStringCheck(tstring stringToCheck);


#ifdef MEMMONITOR
	DWORD SpaceMonitor(LPVOID dummy);
#endif

};


//Map for MemoryRegion -> fileName.bin 
	struct memory_map
			{
				LARGE_INTEGER start;
				LARGE_INTEGER end;
				LARGE_INTEGER size;
				int type;
				TCHAR fileName[3][30];
				bool final;
			};
	
//Map for MemoryRegion -> fileName.bin

#define FUNC_SUCCESS 0
//Registry related errors
#define ERR_REG_OPEN_KEY -101
#define ERR_REG_QUERY_INFO -102
#define ERR_REG_ENUM_VALUE -103
#define ERR_VALUE_NOT_FOUND -104
//Fatal errors

#define ERR_REPOSITORY_OPEN -201