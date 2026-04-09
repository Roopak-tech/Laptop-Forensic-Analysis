// WSTMain.cpp : Defines the entry point for the console application.
//

#define _UNICODE
#define UNICODE


#include "stdafx.h"
#include "WSTLog.h"
#include "WSTRemote.h"
#include "usb_serial_validate.h"
#include "SysCallsLog.h"
#include "resource.h"
#include <sys/types.h>
#include <sys/stat.h>

#ifdef PRO
#include <shlobj.h>
#define WM_USERMEDIA_CHANGED WM_USER+88
#endif

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include "hfuncs.h"
#include "windows.h"


extern int UnmountAndEjectToken(char driveLetter);
#ifdef PRO
TCHAR caseDrive[10];//The drive selected by the user to store all the acquisition files.
TCHAR uslattDrive[256];//The drive uslatt is running from
#endif
gui_resources gui;//This variable will be used globally in US-LATT and passed to DLLs. Add any gui componenet to this.
DWORD WINAPI StartUSLATT(void *);

#ifdef _DEBUG
   WSTLog AppLog(_T("AuditReport_incomplete.xml"), WSL_NEW | WSL_VERBOSE);
#else
#ifdef PRO
#ifdef CMDLINE
WSTLog AppLog((tstring(caseDrive)+_T("\\Windows\\US-LATT\\AuditReport_incomplete.xml")).c_str(), WSL_NEW);
#else
WSTLog AppLog((tstring(caseDrive)+_T("AuditReport_incomplete.xml")).c_str(), WSL_NEW);
#endif
#else
   WSTLog AppLog(_T("AuditReport_incomplete.xml"), WSL_NEW);
#endif
#endif // _DEBUG

//extern list<char> trueCryptDrives;

#ifndef _DEBUG
int SafelyUnmountDrive()
{

	ULONGLONG totalAvailSpace;
	INC_SYS_CALL_COUNT(GetDiskFreeSpaceEx); // Needed to be INC TKH
	GetDiskFreeSpaceEx(NULL,NULL,(PULARGE_INTEGER)&totalAvailSpace,NULL);
	
	const ULONGLONG kangEight=8589934592; //Check less than in this case
	const ULONGLONG kangSixtyFour=64424509440; //Check greater than in this case
	
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;

	INC_SYS_CALL_COUNT(ZeroMemory); // Needed to be INC TKH
	ZeroMemory(&si,sizeof(si));
	si.cb=sizeof(si);
	INC_SYS_CALL_COUNT(ZeroMemory); // Needed to be INC TKH
	ZeroMemory(&pi,sizeof(pi));

	LPSTR cmdline=NULL;//="SafelyRemoveV2.exe";
	//ZeroMemory(cmdline,MAX_PATH);

	cmdline="SafelyRemoveV2.exe";


/*	if(totalAvailSpace < kangEight) //V2 token
	{
		cmdline="SafelyRemoveV2.exe";
		//strncpy(cmdline,"SafelyRemoveV2.exe",MAX_PATH);
	}
*/	
	si.dwFlags=STARTF_USESHOWWINDOW;
	if(!CreateProcessA(
				  NULL,
				  cmdline,
				  NULL,
				  NULL,
				  FALSE,
				  NULL,
				  NULL,
				  NULL,
				  &si,
				  &pi))
	{
		TCHAR msg[MAX_PATH];
		INC_SYS_CALL_COUNT(ZeroMemory); // Needed to be INC TKH
		ZeroMemory(msg,MAX_PATH);
		_stprintf_s(msg,MAX_PATH,L"Physically unmount the encrypted parition from taskbar \n");
		MessageBox(NULL,msg,L"Info",MB_OK);
	}
	//Don't wait
	INC_SYS_CALL_COUNT(WaitForSingleObject); // Needed to be INC TKH
	WaitForSingleObject(pi.hProcess,0 );

    // Close process and thread handles. 
	INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
    CloseHandle(pi.hProcess );
    CloseHandle(pi.hThread );
	return 0;
}
#else 
int SafelyUnmountDrive()
{
	return 0;
}
#endif

/*
Changes for US-LATT Pro Feature (ability to add external drive) - RJM
4/3/2012 10:47:37 AM
*/
#ifdef PRO
typedef struct {
    DWORD dwItem1;    // dwItem1 contains the previous PIDL or name of the folder. 
    DWORD dwItem2;    // dwItem2 contains the new PIDL or name of the folder. 
} SHNOTIFYSTRUCT;
HWND hwndDriveList =NULL;
HWND hwndTip=NULL;
HWND hwndTool=NULL;
int ItemIndex=0;
TCHAR cItem[MAX_PATH+1+10];
TCHAR  ListItem[10];
const unsigned char StateSize=MAX_PATH*4;
TCHAR DriveState[StateSize];
COMBOBOXINFO cbi={0};
TRACKMOUSEEVENT tme={0};
BOOLEAN activateStart=FALSE;
SHChangeNotifyEntry scne[1];
TCHAR driveChanged[10];
SHNOTIFYSTRUCT *shns;
int driveIndex=0;
char defaultSel=-1;
TCHAR volBuffer[MAX_PATH+1];
TCHAR fsType[40];
TCHAR driveEntry[MAX_PATH+1+10];
TOOLINFO ti= {0};
#endif


//In order to perform physical memory cleanup during abort
extern HANDLE hBin;
extern SC_HANDLE hSCM;
extern SC_HANDLE hSVC;
extern HANDLE hphysmemDevice;
extern bool cleanupDuringAbort;
void CleanUpOnExitFromDumpMemory64(IN HANDLE hBin=NULL,IN SC_HANDLE hSCM=NULL,IN SC_HANDLE hSVC=NULL,IN HANDLE hphysmemDevice = INVALID_HANDLE_VALUE);
CDPI g_metrics;//DPI object

#ifdef DRIVE_IMAGING
int ctrlId=1;//Provides incremental control identifier for all the check box buttons.
ULONGLONG physMem=0;
#endif
//HWND htext;





#ifdef PRO

// This logic needs to be documented- This function gets called in 2 places. Before any of these calls the "fixed warnings" (ones that will remain until the apllication has been restarted, presumably in admin mode) will 
// have occurred. Now the static variable in the function warningCount contains the count of these fixed warnings. So whenever warnings are added this count remains a constant and will not change. THe warnings that
// get added in this function are "removable warnings" (ones that go away in the current run of the tool if appropriate measures are taken). In the last else condition warnings are removed if the FAT/FAT32 warnings are
// resolved. The fixed warnings however will not be removed because of the while loop constraint.
// At the moment the "fixed warnings" are
// a. Non admin mode physical memory capture is not possible.
// b. Non admin mode drive imaging is disabled

//isStartEnabled is used to check if the start button is enabled. Flash the physmem warning only if the drive is usable as US-LATT drive

//Keep and eye out for this logic in the future --RJM

void CheckFileSystemType(TCHAR *fsType,ULONGLONG &totalFreeSpace,BOOLEAN isStartEnabled)
{
	static int warningCount=-1;

	if(warningCount == -1)//Very first time when we enter set the local warningCount
	{
		warningCount = gui.warningCount;
	}

	//Remove all the "removable warnings" first while keeping the fixed warnings
	while(gui.warningCount != warningCount)
	{
		SendMessage(gui.statusMsgLB,LB_DELETESTRING,(WPARAM)(gui.warningCount - 1),NULL);
		gui.warningCount -= 1;
	}

	gui.EnableWarningButton();
	if(gui.warningCount == 0)
	{
		gui.DisableWarningButton();
	}
	

	if((isStartEnabled) && (physMem >= totalFreeSpace))
	{
			gui.DisplayWarning(_T("Physical memory will not be captured due to insufficient space on the secondary drive"));
			gui.EnableWarningButton();
	}
	if(!_tcscmp(fsType,_T("FAT32")))
	{
			gui.DisplayWarning(_T("The current storage drive selected is FAT32 formatted, files over 4Gb in size will not be captured. Consider using NTFS filesystem."));
			gui.EnableWarningButton();
		
	}
	else if(!_tcscmp(fsType,_T("FAT")))
	{
		gui.DisplayWarning(_T("This drive is FAT formatted, files over 2Gb in size will not be captured. Consider using NTFS filesystem."));
		gui.EnableWarningButton();
	}
	/*else
	{
		while(gui.warningCount != warningCount)
		{
			SendMessage(gui.statusMsgLB,LB_DELETESTRING,(WPARAM)(gui.warningCount - 1),NULL);
			gui.warningCount -= 1;
		}
		
		
		gui.EnableWarningButton();
		if(gui.warningCount == 0)
		{
			gui.DisableWarningButton();
		}
	}*/



}
//This function takes as input the handle to the combo box and uses the current selected entry to bring up a popup window that would display
//the drive properties
void DetermineDriveProperties(HWND cHwnd)
{


	activateStart=FALSE;
	int len=0;
	memset(cItem,0,MAX_PATH+1+10);
	memset(ListItem,0,10);
	SetErrorMode(SEM_FAILCRITICALERRORS);
	memset(DriveState,0,StateSize*sizeof(TCHAR));
	ItemIndex=0;

	ItemIndex = SendMessage(cHwnd, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
	(TCHAR) SendMessage(cHwnd, (UINT) CB_GETLBTEXT,(WPARAM) ItemIndex, (LPARAM) cItem);

	_tcsncpy_s(ListItem,10,cItem,3);//The last three represent the root of the drive.

	//MessageBox(NULL,ListItem,NULL,MB_OK);

	_sntprintf_s(DriveState,StateSize*sizeof(TCHAR),StateSize*sizeof(TCHAR),_T("Drive Name: %s\n"),ListItem);


	len=_tcslen(DriveState);
	ULARGE_INTEGER bytesAvailable;
	memset(&bytesAvailable,0,sizeof(ULARGE_INTEGER));
	GetDiskFreeSpaceEx(ListItem,&bytesAvailable,NULL,NULL);
	_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Space Available: %lld MB\n"),bytesAvailable.QuadPart/(1024*1024));

	len=_tcslen(DriveState);
	switch(GetDriveType(ListItem))
	{
	case DRIVE_REMOVABLE:
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Drive Type: REMOVABLE\n"));
		len=_tcslen(DriveState);
		if(bytesAvailable.QuadPart == 0)
		{
			_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Usable as US-LATT Drive : NO\n"));

		}
		else
		{							
			activateStart=TRUE;
			_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Usable as US-LATT Drive : YES\n"));
		}
		break;
	case DRIVE_FIXED:

		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Drive Type: FIXED\n"));
		len=_tcslen(DriveState);
		if(bytesAvailable.QuadPart == 0)
		{

			_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Usable as US-LATT Drive : NO\n"));
		}
		else
		{
			activateStart=TRUE;
			_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Usable as US-LATT Drive : YES\n"));
		}

		break;
	case DRIVE_REMOTE:
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Drive Type: REMOTE\n"));
		len=_tcslen(DriveState);
		if(bytesAvailable.QuadPart == 0)
		{
			_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Usable as US-LATT Drive : NO\n"));
		}
		else
		{
			activateStart=TRUE;
			_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Usable as US-LATT Drive : YES\n"));
		}
		break;
	case DRIVE_CDROM:
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Drive Type: CD-ROM\n"));
		len=_tcslen(DriveState);
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Usable as US-LATT Drive : NO\n"));
		break;
	case DRIVE_UNKNOWN:
	case DRIVE_NO_ROOT_DIR:
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Drive Type: UKNOWN\n"));
		len=_tcslen(DriveState);
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Usable as US-LATT Drive : NO\n"));
		break;
	case DRIVE_RAMDISK:
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Drive Type: RAM DISK\n"));
		len=_tcslen(DriveState);
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Usable as US-LATT Drive : NO\n"));
		break;
	default:
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Drive Type: INVALID\n"));
		len=_tcslen(DriveState);
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Usable as US-LATT Drive : NO\n"));
	}


	len=_tcslen(DriveState);
	TCHAR volName[MAX_PATH+1],fsType[20];
	DWORD systemFlags;
	memset(volName,0,(MAX_PATH+1)*sizeof(TCHAR));
	memset(fsType,0,20*sizeof(TCHAR));
	GetVolumeInformation(ListItem,volName,MAX_PATH,NULL,NULL,&systemFlags,fsType,20);
	_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Volume Name: %s\n"),volName);
	len=_tcslen(DriveState);
	_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("FileSystem: %s\n"),fsType);

	len=_tcslen(DriveState);
	if(systemFlags & FILE_READ_ONLY_VOLUME)
	{
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Read Only: YES\n"));
		if(activateStart == TRUE)
		{
			activateStart = FALSE;
		}
	}
	else
	{
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Read Only: NO\n"));
	}


	CheckFileSystemType(fsType,(bytesAvailable.QuadPart),activateStart);


	ti.cbSize   =  sizeof(TOOLINFO);
	ti.uFlags   =  TTF_IDISHWND  |TTF_TRACK|TTF_ABSOLUTE;
	ti.hwnd     =  gui.DialogHwnd;
	ti.hinst    =  gui.hInst;
	ti.uId      =  (UINT_PTR)gui.DialogHwnd;
	ti.lpszText = DriveState;
	SendMessage(hwndTip, TTM_SETMAXTIPWIDTH, 0,3000);

	RECT driveRect;
	GetWindowRect(hwndDriveList,&driveRect);
	POINT pt={driveRect.right,driveRect.top};

	SendMessage(hwndTip,TTM_ADDTOOL,0,(LPARAM)&ti);
	SendMessage(hwndTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(pt.x+50,pt.y));
	SendMessage(hwndTip, TTM_TRACKACTIVATE, (WPARAM)TRUE, (LPARAM)&ti);
	SendMessage(hwndTip,TTM_SETTITLE,TTI_INFO,(LPARAM)_T("Device Properties"));
	SendMessage(hwndTip, TTM_SETTOOLINFO, (WPARAM)TRUE, (LPARAM)&ti);

	//Enable the start button only when the the drive is usable as a US-LATT storage drive
	if(activateStart)
	{
		EnableWindow(gui.hstartButton,TRUE);
#ifdef DRIVE_IMAGING
		gui.EnableDriveImageButton();
#endif
	}
	else
	{
		EnableWindow(gui.hstartButton,FALSE);
#ifdef DRIVE_IMAGING
		gui.DisableDriveImageButton();
#endif
	}

}


void GetDriveList(int  refresh=0)
{
	unsigned char numDriveStrings=0;
	TCHAR *driveList = new TCHAR[MAX_PATH+1];
	
	TCHAR *drivePtr=NULL;
	memset(driveList,0,sizeof(TCHAR)*(MAX_PATH+1));


	if(!refresh)
	{
		//Create the static text asking the user to select the drive
			CreateWindowEx(0,
				_T("STATIC"),
				_T("Select Evidence Storage"),
				WS_CHILD | WS_VISIBLE,
				(gui.stateRect.right-g_metrics.ScaleX(210)),
				(gui.stateRect.bottom-g_metrics.ScaleY(130)),
				g_metrics.ScaleX(180),
				g_metrics.ScaleY(60),
				gui.DialogHwnd,
				NULL,
				gui.hInst,
				NULL);

			//Create the combo box window element that holds all the mounted tokens.
			hwndDriveList = CreateWindowEx(0L,
				_T("COMBOBOX"),
				_T("StorageDriveList"),
				WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|CBS_DROPDOWNLIST|CBS_SORT,
				(gui.stateRect.right-g_metrics.ScaleX(200)),
				(gui.stateRect.bottom-g_metrics.ScaleY(110)),
				g_metrics.ScaleX(136),
				g_metrics.ScaleY(100),
				gui.DialogHwnd,
				(HMENU)IDC_MAIN_PROGRESS,
				gui.hInst,
				NULL);

	}
			if( (numDriveStrings = GetLogicalDriveStrings(MAX_PATH,driveList)) > MAX_PATH)
			{
				//Need more space to collect the drive strings
				delete []driveList;
				driveList=new TCHAR[numDriveStrings+1];
				memset(driveList,0,sizeof(TCHAR)*(numDriveStrings+1));
				GetLogicalDriveStrings(numDriveStrings,driveList);
			}

			drivePtr = driveList;

			SetErrorMode(SEM_FAILCRITICALERRORS);//In case of in-accessible drives.
			do
			{
				memset(volBuffer,0,MAX_PATH+1);
				memset(fsType,0,40);
				memset(driveEntry,0,MAX_PATH+1+10);
				GetVolumeInformation(drivePtr,volBuffer,MAX_PATH,NULL,NULL,NULL,fsType,40);

				_sntprintf_s(driveEntry,MAX_PATH+10,MAX_PATH+1,_T("%s  %s"),drivePtr,volBuffer);//The space here matter in the the _tcsncmp call below.
				//Add the drive to the ComboBox
				SendMessage(hwndDriveList,
							CB_ADDSTRING,
							0,
							reinterpret_cast<LPARAM>((LPCTSTR)driveEntry));
				
				if( !_tcsncmp(&driveEntry[5],_T("WSTSTOR"),7))//The hardcoded numbers depend on the _snprintf_s above
				{
					defaultSel=driveIndex;
				}

				if( (defaultSel == -1) && !_tcsncmp(&driveEntry[5],_T("US-LATT"),7))//The hardcoded numbers depend on the _snprintf_s above
				{
					defaultSel=driveIndex;
				}
				driveIndex++;
 				while(*drivePtr++);//Advance until we hit a null character

			}while(*drivePtr);

			
			delete  []driveList;
}
#endif


#ifdef DRIVE_IMAGING

TCHAR * ConvertToAppropriateBytePrefix(TCHAR *message,ULARGE_INTEGER &totalNumberOfBytes,LONGLONG physicalMemory=0)
{
	TCHAR approriateMessage[MAX_PATH+1];

	memset(approriateMessage,0,sizeof(TCHAR)*(MAX_PATH+1));

	if(totalNumberOfBytes.QuadPart < physicalMemory)
	{
		_sntprintf_s(approriateMessage,(MAX_PATH)*sizeof(TCHAR),_TRUNCATE,_T("%s 0 bytes\n"),message);
		return approriateMessage;

	}
	if((totalNumberOfBytes.QuadPart - physicalMemory) >= GB1)
	{
		_sntprintf_s(approriateMessage,(MAX_PATH)*sizeof(TCHAR),_TRUNCATE,_T("%s %.2llf GB\n"),message,(double)(totalNumberOfBytes.QuadPart - physicalMemory)/GB1);
	}
	else if((totalNumberOfBytes.QuadPart - physicalMemory) >= MB1)
	{
		_sntprintf_s(approriateMessage,(MAX_PATH)*sizeof(TCHAR),_TRUNCATE,_T("%s %.2llf MB\n"),message,(double)(totalNumberOfBytes.QuadPart- physicalMemory)/MB1);
	}
	else if((totalNumberOfBytes.QuadPart - physicalMemory) >= KB1)
	{
		_sntprintf_s(approriateMessage,(MAX_PATH)*sizeof(TCHAR),_TRUNCATE,_T("%s %.2llf KB\n"),message,(double)(totalNumberOfBytes.QuadPart - physicalMemory)/(KB1));
	}
	else
	{
			_sntprintf_s(approriateMessage,(MAX_PATH)*sizeof(TCHAR),_TRUNCATE,_T("%s %.2llf bytes\n"),message,(double)(totalNumberOfBytes.QuadPart- physicalMemory));
	}

	return approriateMessage;
}

// This function takes as input the name if the drive and returns the device properties
// In a secondary role when the default argument is changed to true. The function returns 
// immediately when it finds that the drive is not "image worthy".

boolean DetermineDriveProperties(TCHAR *driveName)
{

	bool copyInfo = false;

	

	int len=0;
	SetErrorMode(SEM_FAILCRITICALERRORS);
	
	

	if((gui.imageDriveMap).find(driveName[0]) != (gui.imageDriveMap.end()) )//check to see if information has already been gathered for this drive
	{
		copyInfo = true;
	}
	else
	{
		driveImaging *newDrive = new driveImaging;
		gui.imageDriveMap[driveName[0]]=newDrive; //Insert a new entry into the map
		
	}


	_tcsncpy_s(ListItem,10,driveName,3);//The last three represent the root of the drive.


	//If the drive letter passed in is the case drive or US-LATT token then return false
	
	if(!_tcsncmp(driveName,gui.currentDropDownDriveString,3)) //The string currentDropDownDriveString is inclusive of the volume label where as the driveName only contains the drive name (C:\)
	{
		gui.imageDriveMap[driveName[0]]->imagingSelected=false; //Set this so that if during the same session the role of a drive is reveresed from being a imaging drive to a storage drive for US-LATT then we flip this flag.
		return false;
	}

	_sntprintf_s(DriveState,StateSize*sizeof(TCHAR),StateSize*sizeof(TCHAR),_T("Drive Name: %s\n"),ListItem);

	len=_tcslen(DriveState);
	ULARGE_INTEGER totalNumbetOfBytes;
	memset(&totalNumbetOfBytes,0,sizeof(ULARGE_INTEGER));
	GetDiskFreeSpaceEx(ListItem,NULL,&totalNumbetOfBytes,NULL);

	if(!copyInfo)
	{
		_tcsncpy_s(gui.imageDriveMap[driveName[0]]->driveName,ListItem,3);
		gui.imageDriveMap[driveName[0]]->size.QuadPart=totalNumbetOfBytes.QuadPart;
	}

	if(totalNumbetOfBytes.QuadPart >= GB1)
	{
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Capacity: %lld Gb\n"),totalNumbetOfBytes.QuadPart/(GB1));
	}
	else if(totalNumbetOfBytes.QuadPart >= MB1)
	{
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Capacity: %lld Mb\n"),totalNumbetOfBytes.QuadPart/(MB1));
	}
	else if(totalNumbetOfBytes.QuadPart >= KB1)
	{
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Capacity: %lld Kb\n"),totalNumbetOfBytes.QuadPart/(KB1));
	}
	else
	{
			_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Capacity: %lld bytes\n"),totalNumbetOfBytes.QuadPart);
	}
	len=_tcslen(DriveState);
	switch(GetDriveType(ListItem))
	{
	case DRIVE_REMOVABLE:
		if(!copyInfo)
		{
			_tcscpy_s(gui.imageDriveMap[driveName[0]]->driveType,10,_T("REMOVABLE"));
		}
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Drive Type: REMOVABLE\n"));
		len=_tcslen(DriveState);
		if(totalNumbetOfBytes.QuadPart == 0)
		{
			return false;
		}
		break;
	case DRIVE_FIXED:
		if(!copyInfo)
		{
			_tcscpy_s(gui.imageDriveMap[driveName[0]]->driveType,10,_T("FIXED"));
		}
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Drive Type: FIXED\n"));
		len=_tcslen(DriveState);
		if(totalNumbetOfBytes.QuadPart == 0)
		{
			return false;
		}
		break;
	case DRIVE_REMOTE:
	case DRIVE_CDROM:
	case DRIVE_UNKNOWN:
	case DRIVE_NO_ROOT_DIR:
	case DRIVE_RAMDISK:
	default:
		return false;
	}


	len=_tcslen(DriveState);
	TCHAR volName[MAX_PATH+1],fsType[20];
	DWORD systemFlags;
	memset(volName,0,(MAX_PATH+1)*sizeof(TCHAR));
	memset(fsType,0,20*sizeof(TCHAR));
	GetVolumeInformation(ListItem,volName,MAX_PATH,NULL,NULL,&systemFlags,fsType,20);

	if( (!_tcsncmp(volName,_T("US-LATT"),7)) || (!_tcsncmp(volName,_T("WSTSTOR"),7)))
	{
		return false;
	}
	_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Volume Name: %s\n"),volName);
	len=_tcslen(DriveState);
	_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("FileSystem: %s\n"),fsType);
	if(!copyInfo)
	{
		_tcscpy_s(gui.imageDriveMap[driveName[0]]->volumeName,MAX_PATH,volName);
		_tcscpy_s(gui.imageDriveMap[driveName[0]]->fileSystem,10,fsType);
	}

	len=_tcslen(DriveState);
	if(systemFlags & FILE_READ_ONLY_VOLUME)
	{
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Read Only: YES\n"));
		return false;
	}
	else
	{
		_sntprintf_s(&DriveState[len],StateSize*sizeof(TCHAR),_TRUNCATE,_T("Read Only: NO\n"));
	}
	
	return true;
}
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DialogProc(HWND, UINT, WPARAM, LPARAM);

HWND CreateDialogBox(HWND);
void RegisterDialogClass(HWND);


void UpdateStaticBoxes(HWND hwnd,TCHAR fsType[40])
{

	RECT windowRect;
	memset(&windowRect,0,sizeof(RECT));
	GetWindowRect(hwnd,&windowRect);

	ULARGE_INTEGER ulPhysMem;
	memset(&ulPhysMem,0,sizeof(ULARGE_INTEGER));
	ulPhysMem.QuadPart=physMem;


	int xDPI=g_metrics.GetDPIX();

	if(xDPI == 96)
	{

		CreateWindowEx(0,
			_T("STATIC"),
			_T("Select Drives to Image"), 
			WS_CHILD | WS_VISIBLE,
			20,10,
			g_metrics.ScaleX(200),
			g_metrics.ScaleY(20),
			hwnd,
			NULL,
			gui.hInst,
			NULL);

		CreateWindowEx(0,
			_T("STATIC"),
			ConvertToAppropriateBytePrefix(_T("Total Bytes to Image:             "),gui.totalBytesToImage),
			WS_CHILD | WS_VISIBLE,
			20,200,
			g_metrics.ScaleX(400),
			g_metrics.ScaleY(40),
			hwnd,
			NULL,
			gui.hInst,
			NULL);

		CreateWindowEx(0,
			_T("STATIC"),
			ConvertToAppropriateBytePrefix(_T("Free Space on Storage Drive :"),gui.totalFreeSpace,physMem),
			WS_CHILD | WS_VISIBLE,
			20,220,
			g_metrics.ScaleX(400),
			g_metrics.ScaleY(40),
			hwnd,
			NULL,
			gui.hInst,
			NULL);


		if(physMem != 0)
		{
			
			CreateWindowEx(0,
			_T("STATIC"),
			ConvertToAppropriateBytePrefix(_T("Space Reserved for Physical Memory Capture :"),ulPhysMem,0),
			WS_CHILD | WS_VISIBLE,
			20,240,
			g_metrics.ScaleX(400),
			g_metrics.ScaleY(20),
			hwnd,
			NULL,
			gui.hInst,
			NULL);
		}

		if(!_tcsncmp(fsType,_T("FAT32"),5))
		{
			CreateWindowEx(0,
			_T("STATIC"),
			_T("Warning: US-LATT storage device selected has FAT32 format, cannot store files larger than 4Gb."),
			WS_CHILD | WS_VISIBLE,
			20,270,
			g_metrics.ScaleX(400),
			g_metrics.ScaleY(50),
			hwnd,
			(HMENU)IDC_STATIC_WARNING,
			gui.hInst,
			NULL);
		}
		if(!_tcsncmp(fsType,_T("FAT"),5))
		{
			CreateWindowEx(0,
			_T("STATIC"),
			_T("Warning: US-LATT storage device selected has FAT format, cannot store files larger than 2Gb."),
			WS_CHILD | WS_VISIBLE,
			20,270,
			g_metrics.ScaleX(400),
			g_metrics.ScaleY(50),
			hwnd,
			(HMENU)IDC_STATIC_WARNING,
			gui.hInst,
			NULL);
		}

	}
	else if (xDPI == 120)
	{
		CreateWindowEx(0,
			_T("STATIC"),
			_T("Select Drives to Image"), 
			WS_CHILD | WS_VISIBLE,
			20,10,
			g_metrics.ScaleX(200),
			g_metrics.ScaleY(20),
			hwnd,
			NULL,
			gui.hInst,
			NULL);

		CreateWindowEx(0,
			_T("STATIC"),
			ConvertToAppropriateBytePrefix(_T("Total Bytes to Image:             "),gui.totalBytesToImage),
			WS_CHILD | WS_VISIBLE,
			20,240,
			g_metrics.ScaleX(400),
			g_metrics.ScaleY(40),
			hwnd,
			NULL,
			gui.hInst,
			NULL);

		CreateWindowEx(0,
			_T("STATIC"),
			ConvertToAppropriateBytePrefix(_T("Free Space on Storage Drive:"),gui.totalFreeSpace,physMem),
			WS_CHILD | WS_VISIBLE,
			20,260,
			g_metrics.ScaleX(400),
			g_metrics.ScaleY(40),
			hwnd,
			NULL,
			gui.hInst,
			NULL);


		if(physMem != 0)
		{

			CreateWindowEx(0,
				_T("STATIC"),
				ConvertToAppropriateBytePrefix(_T("Space Reserved for Physical Memory Capture :"),ulPhysMem,0),
				WS_CHILD | WS_VISIBLE,
				20,280,
				g_metrics.ScaleX(400),
				g_metrics.ScaleY(20),
				hwnd,
				NULL,
				gui.hInst,
			    NULL);
		}

		if(!_tcsncmp(fsType,_T("FAT32"),5))
		{
			CreateWindowEx(0,
			_T("STATIC"),
			_T("Warning: US-LATT storage device selected has FAT32 format, cannot store files larger than 4Gb."),
			WS_CHILD | WS_VISIBLE,
			20,320,
			g_metrics.ScaleX(400),
			g_metrics.ScaleY(50),
			hwnd,
			(HMENU)IDC_STATIC_WARNING,
			gui.hInst,
			NULL);
		}
		if(!_tcsncmp(fsType,_T("FAT"),5))
		{
			CreateWindowEx(0,
			_T("STATIC"),
			_T("Warning: US-LATT storage device selected has FAT format, cannot store files larger than 2Gb."),
			WS_CHILD | WS_VISIBLE,
			20,320,
			g_metrics.ScaleX(400),
			g_metrics.ScaleY(50),
			hwnd,
			(HMENU)IDC_STATIC_WARNING,
			gui.hInst,
			NULL);
		}

	}

}


LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	//In the windows callback create the Dialog box that will contain the drive enumeration.
	//with the window as the parent. Also create the button


	// Get the current selection in the drop-down before we disable that control. That particular drive
	// that is currently selected as the secondary device should not be present in the imaging selection.

	int currentSelection = SendMessage(hwndDriveList,CB_GETCURSEL,(WPARAM)0,(LPARAM)0);
	TCHAR driveString[MAX_PATH+1];
	TCHAR *drive= new TCHAR[10];
	
	static HBRUSH hbrBkgnd=NULL;

	SendMessage(hwndDriveList,CB_GETLBTEXT,(WPARAM)(currentSelection),(LPARAM)(&gui.currentDropDownDriveString)); //Get the string from the index
	//Disable access to DriveList, StartButton and Drive Image Button. Enable them when we exit.
	EnableWindow(hwndDriveList,FALSE); 
	gui.DisableStartButton();
	gui.DisableDriveImageButton();

	int xDPI=g_metrics.GetDPIX();

	RECT windowRect;
	memset(&windowRect,0,sizeof(RECT));
	GetWindowRect(hwnd,&windowRect);

	

	switch(msg)  
	{
	case WM_CREATE:
		memset(&(gui.totalBytesToImage),0,sizeof(ULARGE_INTEGER));
		memset(&(gui.totalFreeSpace),0,sizeof(ULARGE_INTEGER));

		memset(drive,0,10*sizeof(TCHAR));
		_tcsncpy(drive,gui.currentDropDownDriveString,3);
		GetDiskFreeSpaceEx(drive,&(gui.totalFreeSpace),NULL,NULL);
		RegisterDialogClass(hwnd);
		gui.hDialogBoxForDriveSelection=CreateDialogBox(hwnd);

		

		if(xDPI == 96)
		{
			CreateWindow(TEXT("button"),_T("Image Drives"),
				WS_VISIBLE | WS_CHILD,
				g_metrics.ScaleX(windowRect.left),g_metrics.ScaleY(windowRect.bottom-180),g_metrics.ScaleX(100), 
				g_metrics.ScaleY(25),        
				hwnd, (HMENU) IDC_BTN_DRIVE_IMAGING_SELECT, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

			CreateWindow(TEXT("button"),_T("Cancel"),
				WS_VISIBLE | WS_CHILD,
				g_metrics.ScaleX(windowRect.left+110),g_metrics.ScaleY(windowRect.bottom-180),g_metrics.ScaleX(100), 
				g_metrics.ScaleY(25),        
				hwnd, (HMENU) IDC_BTN_DRIVE_IMAGING_CANCEL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		}
		else if(xDPI == 120)
		{

			CreateWindow(TEXT("button"),_T("Image Drives"),
				WS_VISIBLE | WS_CHILD,
				120,400,g_metrics.ScaleX(100), 
				g_metrics.ScaleY(25),        
				hwnd, (HMENU) IDC_BTN_DRIVE_IMAGING_SELECT, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

			CreateWindow(TEXT("button"),_T("Cancel"),
				WS_VISIBLE | WS_CHILD,
				260,400,g_metrics.ScaleX(100), 
				g_metrics.ScaleY(25),        
				hwnd, (HMENU) IDC_BTN_DRIVE_IMAGING_CANCEL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
		}

		//Check to see if the secondary storage device is FAT/FAT32 formatted (if so display a warning)
		memset(fsType,0,40*sizeof(TCHAR));
		GetVolumeInformation(drive,NULL,MAX_PATH,NULL,NULL,NULL,fsType,40);

		UpdateStaticBoxes(hwnd,fsType);
		delete []drive;
		break;
	case WM_COMMAND:
		switch(wParam)
		{
		case IDC_BTN_DRIVE_IMAGING_SELECT:
			DestroyWindow(gui.hDriveImageWindow);
			return 0;

		case IDC_BTN_DRIVE_IMAGING_CANCEL:
			gui.totalBytesToImage.QuadPart=0;
			gui.totalFreeSpace.QuadPart=0;
			DestroyWindow(gui.hDriveImageWindow);
			return 0;
		default:
			break;
		}


	case WM_CTLCOLORSTATIC:
		{
			if((HWND)lParam == GetDlgItem(hwnd,IDC_STATIC_WARNING))
			{
				
				HDC hdcStatic = (HDC)wParam;
				
				SetTextColor(hdcStatic,RGB(255,0,0));
				int color = GetSysColor(CTLCOLOR_DLG);

				SetBkColor(hdcStatic,RGB(GetRValue(color),GetGValue(color),GetBValue(color)));

/*
				if(hbrBkgnd == NULL)
				{
					hbrBkgnd = CreateSolidBrush(GetStockObject(SYSTEM_FONT));
				}*/

				return INT_PTR(GetSysColorBrush(CTLCOLOR_DLG));
				
			}
			else
			{
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}

		}
		
	case WM_DESTROY:
		{
			DeleteObject(hbrBkgnd);
			PostQuitMessage(0);
			return 0;
		}
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

//This Dialog callback function is called for drive selection.
LRESULT CALLBACK DialogProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	unsigned char numDriveStrings=0;
	ULARGE_INTEGER capacity;
	TCHAR *driveList = new TCHAR[MAX_PATH+1];
	TCHAR *drivePtr=NULL;
	TCHAR driveString[MAX_PATH+1];
	TCHAR *driveDesc = new TCHAR[4*MAX_PATH+1];
	SCROLLINFO si;

	int x=20;
	int y=20;
	
	int currCtrlid=0; 
	HWND lbHwnd;
	static HBRUSH hbrBkgnd=NULL;
	static int xPos;        // current horizontal scrolling position 
	static int yPos;        // current vertical scrolling position 

	memset(driveList,0,sizeof(TCHAR)*(MAX_PATH+1));
	memset(driveString,0,sizeof(TCHAR)*(MAX_PATH+1));
	memset(driveDesc,0,sizeof(TCHAR)*(4*MAX_PATH+1));

	TCHAR *drive = new TCHAR[10];
	memset(drive,0,10*sizeof(TCHAR));
	_tcsncpy(drive,gui.currentDropDownDriveString,3);

	int xDPI=g_metrics.GetDPIX();
	RECT windowRect;
	memset(&windowRect,0,sizeof(RECT));
	GetWindowRect(gui.hDriveImageWindow,&windowRect);

	if( (numDriveStrings = GetLogicalDriveStrings(MAX_PATH,driveList)) > MAX_PATH)
	{
		//Need more space to collect the drive strings
		delete []driveList;
		driveList=new TCHAR[numDriveStrings+1];
		memset(driveList,0,sizeof(TCHAR)*(numDriveStrings+1));
		GetLogicalDriveStrings(numDriveStrings,driveList);
	}

	switch(msg)  
	{
	case WM_CREATE:
		drivePtr = driveList;
		SetErrorMode(SEM_FAILCRITICALERRORS);//In case of in-accessible drives.
		do
		{
			memset(volBuffer,0,MAX_PATH+1);
			memset(fsType,0,40);
			GetVolumeInformation(drivePtr,volBuffer,MAX_PATH,NULL,NULL,NULL,fsType,40);

			_sntprintf_s(driveDesc,4*MAX_PATH,4*MAX_PATH,_T("%s  %s"),drivePtr,volBuffer);//The space here matter in the the _tcsncmp call below.

			if(DetermineDriveProperties(drivePtr))
			{
				
				CreateWindow(_T("button"),driveDesc,
					WS_VISIBLE | WS_CHILD | BS_CHECKBOX,
					x, y, 185, 35,        
					hwnd, (HMENU) ctrlId, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
				ctrlId++;
				y=y+30;

				//Remember the state when the Image Dialog box is spawned again during the same session
				if(gui.imageDriveMap.find(drivePtr[0]) != gui.imageDriveMap.end())//Check to see if we have an entry for the drive in our map
				{
					if(gui.imageDriveMap[drivePtr[0]]->imagingSelected)
					{
						if(gui.totalFreeSpace.QuadPart >= (gui.imageDriveMap[drivePtr[0]]->size.QuadPart+physMem)) //Take physical memory into account
						{
							CheckDlgButton(hwnd,ctrlId-1,BST_CHECKED);
							gui.totalBytesToImage.QuadPart += gui.imageDriveMap[drivePtr[0]]->size.QuadPart;
							gui.totalFreeSpace.QuadPart    -= gui.imageDriveMap[drivePtr[0]]->size.QuadPart;
						}
						else//Don't check the box and set the imaging flag in the struct to false
						{
							gui.imageDriveMap[drivePtr[0]]->imagingSelected=false;
						}
					}
				}
			}
			driveIndex++;
			

			while(*drivePtr++);//Advance until we hit a null character

		}while(*drivePtr);

		
		
		delete  []driveList;
		delete  []driveDesc;
		break;

	case WM_COMMAND:
		//When an item is checked handle it here. --RJM

		if(HIWORD(wParam) == BN_CLICKED)
		{
			currCtrlid=GetDlgCtrlID(HWND(lParam));
			memset(driveString,0,sizeof(TCHAR)*(MAX_PATH+1));
			GetWindowText((HWND)(lParam),driveString,10);

			if(IsDlgButtonChecked(hwnd,currCtrlid) == BST_CHECKED)//Uncheck it and adjust values
			{
				gui.imageDriveMap[driveString[0]]->imagingSelected=false;
				CheckDlgButton(hwnd,currCtrlid,BST_UNCHECKED);
				gui.totalBytesToImage.QuadPart -= gui.imageDriveMap[driveString[0]]->size.QuadPart;
				gui.totalFreeSpace.QuadPart	   += (gui.imageDriveMap[driveString[0]]->size.QuadPart);
			}
			else //Check it and update values
			{
				
				CheckDlgButton(hwnd,currCtrlid,BST_CHECKED);
				if(gui.totalFreeSpace.QuadPart < (gui.imageDriveMap[driveString[0]]->size.QuadPart+physMem))//Take into account the physical memory that will be captured
				{
					MessageBox(hwnd,_T("The US-LATT storage drive does not have enough free space to save the image of the selected drive. Select a drive with higher capacity under \"Select Evidence Storage\" and then select drives to image."),_T("Space Exceeded"),MB_OK|MB_ICONERROR);
					CheckDlgButton(hwnd,currCtrlid,BST_UNCHECKED);
				}
				else
				{
					gui.imageDriveMap[driveString[0]]->imagingSelected=true;
					gui.totalFreeSpace.QuadPart    -= gui.imageDriveMap[driveString[0]]->size.QuadPart;
					gui.totalBytesToImage.QuadPart += gui.imageDriveMap[driveString[0]]->size.QuadPart;
				}

			}
			
			DetermineDriveProperties(driveString);


			if(xDPI == 96)
			{
			CreateWindowEx(0,
				_T("STATIC"),
				DriveState,
				WS_CHILD | WS_VISIBLE,
				240,25,
				g_metrics.ScaleX(280),
				g_metrics.ScaleY(140),
				gui.hDriveImageWindow,
				NULL,
				gui.hInst,
				NULL);
			}
			else if (xDPI ==120)
			{
				CreateWindowEx(0,
				_T("STATIC"),
				DriveState,
				WS_CHILD | WS_VISIBLE,
				280,35,
				g_metrics.ScaleX(280),
				g_metrics.ScaleY(140),
				gui.hDriveImageWindow,
				NULL,
				gui.hInst,
				NULL);
			}

		
		UpdateStaticBoxes(gui.hDriveImageWindow,fsType);
		}
		break;

		/*case WM_CTLCOLORSTATIC:
		{
			if((HWND)lParam == GetDlgItem(gui.hDriveImageWindow,IDC_STATIC_WARNING))
			{
				HDC hdcStatic = (HDC)wParam;
				SetTextColor(hdcStatic,RGB(255,0,0));
				SetBkColor(hdcStatic,RGB(0,0,0));

				if(hbrBkgnd == NULL)
				{
					hbrBkgnd = CreateSolidBrush(RGB(0,0,0));
				}

				return INT_PTR(hbrBkgnd);
			}
			else
			{
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}

		}
*/
	case WM_VSCROLL:
		// Get all the vertial scroll bar information.
		si.cbSize = sizeof (si);
		si.fMask  = SIF_ALL;
		GetScrollInfo (hwnd, SB_VERT, &si);

		// Save the position for comparison later on.
		yPos = si.nPos;
		switch (LOWORD (wParam))
		{

			// User clicked the HOME keyboard key.
		case SB_TOP:
			si.nPos = si.nMin;
			break;

			// User clicked the END keyboard key.
		case SB_BOTTOM:
			si.nPos = si.nMax;
			break;

			// User clicked the top arrow.
		case SB_LINEUP:
			si.nPos -= 1;
			break;

			// User clicked the bottom arrow.
		case SB_LINEDOWN:
			si.nPos += 1;
			break;

			// User clicked the scroll bar shaft above the scroll box.
		case SB_PAGEUP:
			si.nPos -= si.nPage;
			break;

			// User clicked the scroll bar shaft below the scroll box.
		case SB_PAGEDOWN:
			si.nPos += si.nPage;
			break;

			// User dragged the scroll box.
		case SB_THUMBTRACK:
			si.nPos = si.nTrackPos;
			break;

		default:
			break; 
		}
		
		// Set the position and then retrieve it.  Due to adjustments
		// by Windows it may not be the same as the value set.
		si.fMask = SIF_POS;
		SetScrollInfo (hwnd, SB_VERT, &si, TRUE);
		GetScrollInfo (hwnd, SB_VERT, &si);

		// If the position has changed, scroll window and update it.
		if (si.nPos != yPos)
		{                    
			ScrollWindow(hwnd, 0, (yPos - si.nPos), NULL, NULL);
			UpdateWindow (hwnd);
		}

		break;
	case WM_CLOSE:
		//DeleteObject(hbrBkgnd);
		delete  [] drive;
		delete  []driveList;
		delete  []driveDesc;
		DestroyWindow(hwnd);
		break;       

	}
	return (DefWindowProc(hwnd, msg, wParam, lParam));

}
void RegisterDialogClass(HWND hwnd) 
{
  WNDCLASSEX wc = {0};
  wc.cbSize           = sizeof(WNDCLASSEX);
  wc.lpfnWndProc      = (WNDPROC) DialogProc;
  wc.hInstance        = gui.hInst;
  wc.hbrBackground    = GetSysColorBrush(COLOR_3DFACE);
  wc.lpszClassName    = TEXT("DialogClass");
  RegisterClassEx(&wc);

}

HWND CreateDialogBox(HWND hwnd)
{
	int xDPI=g_metrics.GetDPIX();

	if(xDPI == 96)
	{

		return CreateWindowEx(WS_EX_CLIENTEDGE,TEXT("DialogClass"), TEXT("Drive List"), 
			WS_CHILD|WS_VISIBLE |WS_TABSTOP|WS_VSCROLL, 20, 30,g_metrics.ScaleX(200),g_metrics.ScaleY(150), 
			hwnd, NULL, gui.hInst,  NULL);
	}
	else if(xDPI == 120)
	{
		return CreateWindowEx(WS_EX_CLIENTEDGE,TEXT("DialogClass"), TEXT("Drive List"), 
			WS_CHILD|WS_VISIBLE |WS_TABSTOP|WS_VSCROLL, 20, 40,g_metrics.ScaleX(200),g_metrics.ScaleY(150), 
			hwnd, NULL, gui.hInst,  NULL);
	}

}
#endif
LRESULT CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hdetailButton;
	gui.DialogHwnd		    =	hwnd;
	gui.statusMsgLB			=	GetDlgItem(hwnd,IDC_SIG_LB);
	gui.msgLB=GetDlgItem(hwnd,IDC_SIG_LB_STATUS);
	gui.herrorButton = GetDlgItem(hwnd,IDC_BTN_ERRORS);
	gui.hwarningButton = GetDlgItem(hwnd,IDC_BTN_WARNINGS);
	gui.hejectButton = GetDlgItem(hwnd,IDC_BTN_EJECT);
	gui.hstartButton = GetDlgItem(hwnd,IDC_BTN_RUN);
	gui.habortButton = GetDlgItem(hwnd,IDC_BTN_ABORT);
#ifdef DRIVE_IMAGING
	gui.hDriveImageButton = GetDlgItem(hwnd,IDC_BTN_DRIVE_IMAGING);
#endif
	hdetailButton=GetDlgItem(hwnd,IDC_BTN_DETAILS);

	int errIndex=0;
	int warningIndex=0;
	static int esearchIndex = -1;
	static int wsearchIndex = -1;
	

	
	INITCOMMONCONTROLSEX cc;
	cc.dwSize=sizeof(INITCOMMONCONTROLSEX);
	cc.dwICC=ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&cc);
	DWORD errStartUSLATT=0;

	RECT srect,drect;

	HANDLE hlattThread;
	DWORD threadId;
	HICON hIcon;
	struct _stat buffer;
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof(statex);
	

	
	static int sizes[]={190,450,-1};//The 350 here is used for progress bar//TODO
	int xDPI=g_metrics.GetDPIX();

	/*
	Changes for US-LATT Pro Feature (ability to add external drive) - RJM
	4/2/2012 9:41:54 AM
	*/
#ifdef PRO	
	
	unsigned char currIndex=0;
#endif

#ifdef DRIVE_IMAGING
	 MSG  dmsg ;    
	 WNDCLASS wc = {0}; // The main drive imaging window that conatins other GUI elements.

#endif

	/*
	Changes for US-LATT Pro Feature (ability to add external drive) - RJM
	4/2/2012 9:41:59 AM
	*/
    switch(msg)
    {
		case WM_INITDIALOG:

#ifdef DRIVE_IMAGING
			if(!IsUserAnAdmin())
			{
				gui.DisableDriveImageButton();
				gui.isUserAdmin=false;
				gui.DisplayWarning(_T("Drive Imaging feature is only available if run under administrative user account"));
				gui.EnableWarningButton();

				SetWindowText(gui.DialogHwnd,_T("US-LATT PRO 2.1.0 (Non-Administrative User)"));
			}
			else
			{
				SetWindowText(gui.DialogHwnd,_T("US-LATT PRO 2.1.0 (Administrative User)"));
			}

			//Next check if physical memory capture has been enabled using the file name state_with_mem.txt

			if(_stat("state_with_mem.txt",&buffer) == 0)
			{
				//memset(&statex,0,sizeof(MEMORYSTATUSEX));
				GlobalMemoryStatusEx(&statex);
				physMem= (_int64)statex.ullTotalPhys;//Keep a record of the physical memory
				if(!gui.isUserAdmin)
				{
					gui.DisplayWarning(_T("Physical memory cannot be acquired with non-administrative user credentials"));
				}
			}
			else
			{
				physMem=0;
			}
#endif
			gui.hStatus = CreateWindowEx(0L,STATUSCLASSNAME,NULL,WS_CHILD |WS_VISIBLE,0,0,0,0,hwnd,(HMENU)IDC_MAIN_STATUS,gui.hInst, NULL);
			GetClientRect(gui.hStatus,&srect);
			GetClientRect(hwnd,&drect);

			SendMessage(gui.hStatus,SB_SETPARTS,sizeof(sizes),(LPARAM)sizes);
			GetClientRect(gui.DialogHwnd,&(gui.stateRect)	);
			
			
			xDPI=g_metrics.GetDPIX();
			
			//itoa(xDPI,Msg,10);
			//MessageBoxA(NULL,Msg,"Hello",MB_OK);
			//19<-21
			//160->200
			if(xDPI == 96)
			{
			gui.hProgress = CreateWindowEx(0L,PROGRESS_CLASS,NULL,WS_CHILD |WS_VISIBLE|PBS_SMOOTH,452,
				(gui.stateRect.bottom-g_metrics.ScaleY(19)),(g_metrics.ScaleX(160)),g_metrics.ScaleY(21)
				,hwnd,(HMENU)IDC_MAIN_PROGRESS,gui.hInst, NULL);
			}
			else if(xDPI == 120)
			{
			gui.hProgress = CreateWindowEx(0L,PROGRESS_CLASS,NULL,WS_CHILD |WS_VISIBLE|PBS_SMOOTH,452,
				(gui.stateRect.bottom-g_metrics.ScaleY(21)),(g_metrics.ScaleX(290)),g_metrics.ScaleY(21)
				,hwnd,(HMENU)IDC_MAIN_PROGRESS,gui.hInst, NULL);
			}
			else//144 or 192
			{
				gui.hProgress = CreateWindowEx(0L,PROGRESS_CLASS,NULL,WS_CHILD |WS_VISIBLE|PBS_SMOOTH,452,
				(gui.stateRect.bottom-g_metrics.ScaleY(22)),(g_metrics.ScaleX(310)),g_metrics.ScaleY(21)
				,hwnd,(HMENU)IDC_MAIN_PROGRESS,gui.hInst, NULL);
			}
			
			SendMessage(gui.statusMsgLB,LB_SETHORIZONTALEXTENT,2000,NULL);//To make the horzontal scrollbar visible

			hIcon = (HICON) LoadImage(gui.hInst,
                           MAKEINTRESOURCE(USLATT_ICON),
						   IMAGE_ICON,
                           32,
                           32,
                           0);

			if(hIcon != NULL)
			{
				SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)(hIcon));
			}
#ifdef PRO
			/*
			Changes for US-LATT Pro Feature (ability to add external drive) - RJM
			4/2/2012 9:30:30 AM
			Setup the options for the user to be able to select the external drive to use for 
			evidence storage.
			*/

			memset(uslattDrive,0,256);

			//Set the global uslattDrive array for furture reference.
			int len;
			int qRes;
			len = GetCurrentDirectory(256,uslattDrive);
			gui.DisplayStatus(_T("usLattDrive"));
			
			/* Let us check it is a network drive
			 */
			char c_szText[256];
			size_t t;
			wcstombs_s(&t,c_szText,256,uslattDrive,wcslen(uslattDrive) + 1);
			if(c_szText[0] != '\\' && c_szText[1] != '\\')
			{
				memset(&uslattDrive[3],0,253);
			}
			else 
			{
				_tcscat_s(uslattDrive,L"\\");
			}
			
			gui.DisplayStatus(uslattDrive);
		
			//Register device insertion and removal notifications with the current window
			SHChangeNotifyRegister(hwnd,SHCNRF_ShellLevel,SHCNE_DRIVEADD|SHCNE_DRIVEREMOVED|SHCNE_MEDIAINSERTED|SHCNE_MEDIAREMOVED,WM_USERMEDIA_CHANGED,1,scne);

		
			(void)GetDriveList();//Get all the currently mounted drives and put them in a combobox
			
			hwndTip = CreateWindowEx(NULL,TOOLTIPS_CLASS, NULL,
                            WS_POPUP,
                            CW_USEDEFAULT, CW_USEDEFAULT,
                            CW_USEDEFAULT, CW_USEDEFAULT,
							gui.DialogHwnd, NULL, gui.hInst,
                            NULL);

			/*Create a default entry*/
			cbi.cbSize=sizeof(COMBOBOXINFO);
			GetComboBoxInfo(hwndDriveList,&cbi);
			SendMessage(hwndDriveList,CB_SETCURSEL,defaultSel,0);


			//Check if the default selection is FAT/FAT32 in which case we report it as a warning
			int defaultIndex;
			defaultIndex=SendMessage(hwndDriveList,CB_GETCURSEL,0,0);
			if(defaultIndex != CB_ERR)
			{
				ULONGLONG totalAvailableSpace ;
				totalAvailableSpace = 0;
				TCHAR *buf = new TCHAR[MAX_PATH+1];
				TCHAR *drive = new TCHAR[10];
				memset(buf,0,sizeof(TCHAR)*(MAX_PATH+1));
				if(SendMessage(hwndDriveList,CB_GETLBTEXT,defaultIndex,(LPARAM)buf) != CB_ERR)
				{
					memset(fsType,0,sizeof(TCHAR)*40);
					memset(drive,0,sizeof(TCHAR)*4);
					_tcsncpy_s(drive,10,buf,3);
					GetVolumeInformation(drive,NULL,NULL,NULL,NULL,NULL,fsType,40);
					GetDiskFreeSpaceEx(drive,(PULARGE_INTEGER)&totalAvailableSpace,0,0);
					CheckFileSystemType(fsType,totalAvailableSpace,true);
					
				}
				delete [] buf;
				delete [] drive;
			}


			
#endif
			return TRUE;

#ifdef PRO
//-----End of case WM_INITDIALOG----//
			/*
			Changes for US-LATT Pro Feature (ability to add external drive) - RJM
			4/3/2012 10:47:37 AM
			Setup the tooltip for the selected drive 
			*/
			//We get this message when a drive is inserted or removed. THe lParam contains the exact nature of the event and we handle the combobox accordingly.
		case WM_USERMEDIA_CHANGED:

			if(gui.isTriageInProgress)//Don't react to device insertions when a triage is in progress
			{
				return TRUE;
			}
			shns = (SHNOTIFYSTRUCT *)wParam;
			switch(lParam)
			{
				case SHCNE_MEDIAINSERTED:
				case SHCNE_DRIVEADD:
					memset(volBuffer,0,MAX_PATH+1);
					memset(fsType,0,40);
					memset(driveChanged,0,10);
					memset(ListItem,0,10);
					
					SHGetPathFromIDList((PCIDLIST_ABSOLUTE)(shns->dwItem1),driveChanged);//Get the drive name from the wParam that has it as an IDLIST
					_tcsncpy_s(ListItem,10,driveChanged,3);//The last three represent the root of the drive.
					
					if (SendMessage(hwndDriveList,CB_FINDSTRING,(WPARAM)-1,(LPARAM)ListItem) == CB_ERR)
					{
						
						GetVolumeInformation(ListItem,volBuffer,MAX_PATH,NULL,NULL,NULL,fsType,40);

						_sntprintf_s(driveEntry,MAX_PATH+10,MAX_PATH+1,_T("%s  %s"),ListItem,volBuffer);//The space here matter in the the _tcsncmp call below.

						defaultSel = SendMessage(hwndDriveList,
							CB_ADDSTRING,
							0,
							reinterpret_cast<LPARAM>((LPCTSTR)driveEntry));
					}

				
					
					SendMessage(hwndDriveList,CB_SETCURSEL,defaultSel,0);
					
					DetermineDriveProperties(cbi.hwndCombo);//Refresh the tooltip and bring it up for the newly insrted drive and set it as default selection
					break;

				case SHCNE_MEDIAREMOVED:
				case SHCNE_DRIVEREMOVED:
					SHGetPathFromIDList((PCIDLIST_ABSOLUTE)(shns->dwItem1),driveChanged);
					_tcsncpy_s(ListItem,10,driveChanged,3);//The last three represent the root of the drive.
					
					//Check to see if the drive is in our list and get the index
					if ( (driveIndex = SendMessage(hwndDriveList,CB_FINDSTRING,(WPARAM)-1,(LPARAM)ListItem)) != CB_ERR)
					{
						SendMessage(hwndDriveList,CB_DELETESTRING,(WPARAM)driveIndex,NULL);
						
					}

					if(SendMessage(hwndDriveList,CB_GETCURSEL,-1,0) == driveIndex)//Do this only when the drive removed is the same as the current selection
					{
						SendMessage(hwndDriveList,CB_SETCURSEL,-1,0);
						SendMessage(hwndTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&ti);
						gui.DisableStartButton();
						if ( (driveIndex = SendMessage(hwndDriveList,CB_FINDSTRING,(WPARAM)-1,(LPARAM)uslattDrive)) != CB_ERR)
						{
							SendMessage(hwndDriveList,CB_SETCURSEL,driveIndex,0);
							DetermineDriveProperties(cbi.hwndCombo);//Refresh trhe tooltip and bring it up for the newly insrted drive and set it as default selection
						}
					}
					
					driveIndex=0;
					break;	
			}
			return TRUE;
//-----End of case WM_USERMEDIA_CHANGED----//
		case WM_ACTIVATE:
			if(LOWORD(wParam) == WA_ACTIVE)
			{
				SendMessage(hwndTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&ti);
				return TRUE;
			}
#endif
		case WM_COMMAND:
			/*
			Changes for US-LATT Pro Feature (ability to add external drive) - RJM
			4/3/2012 10:47:37 AM
			Setup the tooltip for the selected drive 
			*/
#ifdef PRO

			if(HIWORD(wParam) == CBN_SELCHANGE)
			{
					
				if((HWND)lParam == hwndDriveList)//This condition checks that we are proessing only when the notification is relevant to the hwndDriveList object.
				{
					DetermineDriveProperties((HWND) lParam);
				}
				return TRUE;
			}

			if(HIWORD(wParam) == CBN_DROPDOWN)//This change is exculsively for WIndows XP the SHChangeNotifyRegister does not see to work properly without administrative privileges.
			{
				SendMessage(hwndTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&ti);
				SendMessage(hwndDriveList,CB_RESETCONTENT,0,0);
#ifdef DRIVE_IMAGING
			EnableWindow(gui.hDriveImageButton,FALSE); 
#endif
				EnableWindow(gui.hstartButton,FALSE); //Bug fixed (if nothing from drop down is selected).
				GetDriveList(1);
				return TRUE;
			}


#endif
			switch(wParam)
			{
			
			case IDC_BTN_DETAILS:
				if(gui.detail == true)
				{
					SetWindowText(hdetailButton,TEXT("<<Hide Details"));
					if(xDPI==96)
					{
						SetWindowPos(hwnd,HWND_TOP,0, 0, g_metrics.ScaleX(618),g_metrics.ScaleY(400),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
					}
					else if(xDPI==120)
					{
						SetWindowPos(hwnd,HWND_TOP,0, 0, g_metrics.ScaleX(658),g_metrics.ScaleY(400),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
					}
					else//144 or 192
					{
						SetWindowPos(hwnd,HWND_TOP,0, 0, g_metrics.ScaleX(618),g_metrics.ScaleY(440),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
					}
					
					UpdateWindow(hwnd);
					gui.detail=false;
				}
				else
				{
					if(xDPI == 96)
					{
						SetWindowPos(hwnd,HWND_TOP,0,0,g_metrics.ScaleX(618),g_metrics.ScaleY(196),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);//196->188
					}
					else if(xDPI==120)
					{
						SetWindowPos(hwnd,HWND_TOP,0, 0,g_metrics.ScaleX(658),g_metrics.ScaleY(196),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);//196->188
					}
					else//144 or 192
					{
						SetWindowPos(hwnd,HWND_TOP,0,0,g_metrics.ScaleX(618),g_metrics.ScaleY(196),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);//196->188
					}
					
					SetWindowText(hdetailButton,TEXT("Details>>"));
					UpdateWindow(hwnd);
					gui.detail=true;
				}
				return TRUE;

			case IDC_BTN_ERRORS:
				
				if(gui.detail == true)
				{
					SetWindowText(hdetailButton,TEXT("<<Hide Details"));
					
					if(xDPI==96)
					{
						SetWindowPos(hwnd,HWND_TOP,0, 0, g_metrics.ScaleX(618),g_metrics.ScaleY(400),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
					}
					else if(xDPI==120)
					{
						SetWindowPos(hwnd,HWND_TOP,0, 0, g_metrics.ScaleX(658),g_metrics.ScaleY(400),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
					}
					else//144 or 192
					{
						SetWindowPos(hwnd,HWND_TOP,0, 0, g_metrics.ScaleX(618),g_metrics.ScaleY(440),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
					}
					
					
					UpdateWindow(hwnd);
					gui.detail=false;
				}
				
				if( (errIndex = SendMessage(gui.statusMsgLB,LB_FINDSTRING,esearchIndex,(LPARAM)(_TEXT("ERROR")))) != LB_ERR)
				{
						SendMessage(gui.statusMsgLB,LB_SETCURSEL,errIndex,NULL);
				}
				else
				{
					MessageBox(0,TEXT("Triage was successful with no errors"),_T("Successful"),MB_OK);
				}
				esearchIndex=errIndex;

				return TRUE;

			case IDC_BTN_WARNINGS:
				
				if(gui.detail == true)
				{
					SetWindowText(hdetailButton,TEXT("<<Hide Details"));
					
					if(xDPI==96)
					{
						SetWindowPos(hwnd,HWND_TOP,0, 0, g_metrics.ScaleX(618),g_metrics.ScaleY(400),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
					}
					else if(xDPI==120)
					{
						SetWindowPos(hwnd,HWND_TOP,0, 0, g_metrics.ScaleX(658),g_metrics.ScaleY(400),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
					}
					else//144 or 192
					{
						SetWindowPos(hwnd,HWND_TOP,0, 0, g_metrics.ScaleX(618),g_metrics.ScaleY(440),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);
					}
					
					
					UpdateWindow(hwnd);
					gui.detail=false;
				}
				
				if( (warningIndex = SendMessage(gui.statusMsgLB,LB_FINDSTRING,wsearchIndex,(LPARAM)(_TEXT("WARNING")))) != LB_ERR)
				{
						SendMessage(gui.statusMsgLB,LB_SETCURSEL,warningIndex,NULL);
				}
				else
				{
					MessageBox(0,TEXT("Triage was successful with no warnings"),_T("Successful"),MB_OK);
				}
				wsearchIndex=warningIndex;

				return TRUE;
			case IDC_BTN_EJECT:
				_fcloseall();//Flush all streams
				SafelyUnmountDrive();
				DestroyWindow(hwnd);//Else Kanguru will fail since there is a open handle to the window.
				return TRUE;
			case IDC_BTN_ABORT:
				//Also kill the thread that is being processed	
				if(cleanupDuringAbort)
				{
					CleanUpOnExitFromDumpMemory64(hBin,hSCM,hSVC,hphysmemDevice);
				}
				_fcloseall();
				SafelyUnmountDrive();
				DestroyWindow(hwnd);
				return TRUE;

#ifdef DRIVE_IMAGING
				// Create a popup window that is pre-populated with drive that can be imaged with
				// US-LATT. The idea is to first have a child window which would contain
				// a dialog box that would be populated with check box buttons corresponding
				// to drives connected to the system. A static window of text will appear when
				// the drive is selected displaying all the properties of the drive. Yet another
				// static text will keep a running count of the total number of Gb of data to
				// be imaged if this count exceeds the free space on the secondary drive then
				// an error will be flagged. The drive image feature will not be enabled
				// until the user selects the secondary drive to collect the evidence on.--RJM

			case IDC_BTN_DRIVE_IMAGING:

				wc.lpszClassName = TEXT( "Window" );
				wc.hInstance     = gui.hInst ;
				wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
				wc.lpfnWndProc   = WndProc;
				
				RegisterClass(&wc);

				//WS_SYSMENU gets you the close option without other window features.
				gui.hDriveImageWindow = CreateWindow( wc.lpszClassName, TEXT("Drive Imaging"),
					WS_SYSMENU| WS_VISIBLE ,
					100, 100, g_metrics.ScaleX(450),g_metrics.ScaleY(400), gui.DialogHwnd, NULL, gui.hInst, NULL);  

				while( GetMessage(&dmsg, NULL, 0, 0)) {
					DispatchMessage(&dmsg);
				}

				EnableWindow(hwndDriveList,TRUE); //Re-enable the drive list access.
				EnableWindow(gui.hstartButton,TRUE); //Re-enable the start Button.
				gui.EnableDriveImageButton();
				return (int) dmsg.wParam;

#endif

			case IDC_BTN_RUN:
#ifdef PRO
				memset(ListItem,0,10);
				memset(cItem,0,MAX_PATH+1+10);
				memset(caseDrive,0,10);
				
				
				SetErrorMode(SEM_FAILCRITICALERRORS);
				//MessageBox(NULL,uslattDrive,NULL,MB_OK);
				//At this point set the case drive that the user selected with a popup message
				if( (currIndex = SendMessage(hwndDriveList,CB_GETCURSEL,0,0)) == CB_ERR)
				{
					MessageBox(NULL,_T("Please select a drive before running triage"),_T("Drive Select Error"),MB_OK|MB_ICONSTOP);
				}
				if(SendMessage(hwndDriveList,CB_GETLBTEXT,(WPARAM)currIndex,(LPARAM)cItem) != CB_ERR)
				{
					_tcsncpy_s(ListItem,10,cItem,3);//The last three represent the root of the drive.

					if(!_tcsncmp(ListItem,uslattDrive,3))
					{
						qRes=MessageBox(NULL,_T("The selected drive will be used during the current US-LATT triage\nAre you sure you want to continue?"),
							_T("Information"),MB_OKCANCEL|MB_ICONQUESTION);
						if(qRes == IDCANCEL)
						{
							return TRUE;
						}
						else
						{
							_tcsncpy_s(caseDrive,10,cItem,3);//The last three represent the root of the drive.
						}
					}
					else
					{
						qRes=MessageBox(NULL,_T("The selected drive will be used during the current US-LATT triage\nSetting volume label to WSTSTOR\nAre you sure you want to continue?")
							,_T("Information"),MB_OKCANCEL|MB_ICONQUESTION);
						if(qRes == IDCANCEL)
						{
							
							return TRUE;
						}
						else
						{
							_tcsncpy_s(caseDrive,10,cItem,3);//The last three represent the root of the drive.
							SetVolumeLabel(caseDrive,_T("WSTSTOR"));
						}
					}
					
				}

				//SendMessage(hwndTip, TTM_TRACKACTIVATE, (WPARAM)FALSE, (LPARAM)&ti);
				EnableWindow(hwndDriveList,FALSE);
#endif
				hlattThread = CreateThread(NULL,0L,StartUSLATT,(void *)&errStartUSLATT,0,&threadId);
				gui.DisableStartButton();
				gui.DisableDriveImageButton();
				gui.DisplayStatus(_T("Case Drive"));
				gui.DisplayStatus(caseDrive);
				return TRUE;
			
			default:
				
				return TRUE;

			}
//-----End of case WM_COMMAND----//
		case WM_CLOSE:
			if(cleanupDuringAbort)
			{
				CleanUpOnExitFromDumpMemory64(hBin,hSCM,hSVC,hphysmemDevice);
			}
			_fcloseall();
#ifndef TESTING
			SafelyUnmountDrive();
#endif
			DestroyWindow(hwnd);
			return TRUE;
//-----End of case WM_CLOSE----//
		case WM_DESTROY:
			PostQuitMessage(0);
			return TRUE;	
//-----End of case WM_DESTROY----//
		
    }
	
	
	return FALSE;
}


#ifndef CMDLINE
//Use nCmdShow for stealth mode activation as minimized
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow)
{
	MSG msg;

	gui.hInst = hInstance;
	//Step 2 create the dialog box
	DialogBox(hInstance,MAKEINTRESOURCE(IDD_MAIN_DLG),0, reinterpret_cast<DLGPROC>(DlgProc));
	gui.uslatt_icon = LoadIconA(hInstance, "USLATT_ICON");
	

	while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
//      TranslateMessage(&msg); //Use this for hotkeys stealth mode
        DispatchMessage(&msg);
		
    }
	

	return (int) msg.wParam;

}
#else

#define PIPENAME _T("USLATTCommPipe")
enum RemoteState
{
	SERVICE_STARTED,
	USLATT_START,
	VALIDATION,
	PHYSICAL_MEMORY,
	SYSTEM_DATA,
	DISK_INFORMATION,
	REGISTRY_COLLECTION,
	SCREENSHOT_CAPTURE,
	INSTALLED_APPLICATIONS,
	EVENTLOG_CAPTURE,
	FILE_COLLECTION,
	NETWORK_INFORMATION,
	USB_DEVICE_HISTORY,
	TRUECRYPT_IMAGING,
	MAIL_FILES_CAPTURE,
	WEB_AND_SKYPE,
	EVENTS_TIMELINE,
	COMPUTING_HASHES,
	USLATT_FINISH,
	SERVICE_FINISH,
	/*-------------------*/
	INDETERMINATE
};

class RemoteStatus
{
private:
	RemoteState state;
	boolean bPercent;//Set/Check when there is a need to monitor percentComplete
	int percentComplete;//Monitor cases were there is a need to check for percentage completeion.
	boolean bFilename;//Set/Check when there is a need to monitor filenames.
	TCHAR fileName[MAX_PATH+1];//When searching for files.
	DWORD bytesWritten;
	HANDLE hPipe;
public:

	RemoteStatus():state(INDETERMINATE),bPercent(false),percentComplete(0),bFilename(false),bytesWritten(0),hPipe(INVALID_HANDLE_VALUE)
	{ 
		memset(fileName,0,sizeof(TCHAR)*(MAX_PATH+1));
	}

	~RemoteStatus()
	{
		if(hPipe != INVALID_HANDLE_VALUE)
		{
			CloseHandle(hPipe);
		}
	}

	void ResetPercentage()
	{
		bPercent=false;
		percentComplete=0;
	}

	void ResetFileName()
	{
		bFilename=false;
		memset(fileName,0,sizeof(TCHAR) * MAX_PATH+1);
	}

	void ResetState()
	{
		state = INDETERMINATE;
	}

	void SetState(RemoteState s)
	{
		state = s;
	}

	void SetFileName(TCHAR *fname)
	{
		memset(fileName,0,sizeof(TCHAR) * MAX_PATH+1);
		bFilename=true;
		_tcscpy_s(fileName,MAX_PATH,fname);
	}

	void SetPercentage(int percentage)
	{
		bPercent=true;
		percentComplete = percentage;
	}

	void SetPipeHandle(TCHAR *handleString)
	{
		hPipe = HANDLE (_tatoi(handleString));		
	}

	RemoteState GetState()
	{
		return state;
	}

	boolean isPercentSet()
	{
		return bPercent;
	}

	int GetPercentage()
	{
		return percentComplete;
	}

	boolean isFilenameSet()
	{
		return bFilename;
	}

	TCHAR * GetFileName()
	{
		return fileName;
	}

	void WriteToPipe()
	{
		WriteFile(hPipe,(void*)this,sizeof(RemoteStatus),&bytesWritten,NULL);
	}
	
};



RemoteStatus remoteStatus;

int _tmain(int argc, TCHAR *argv[])
{
	int errCode=0;
	TCHAR currDrive[MAX_PATH+1];
	memset(currDrive,0,(MAX_PATH+1)*sizeof(TCHAR));
	memset(caseDrive,0,sizeof(TCHAR)*10);
	GetCurrentDirectory(MAX_PATH,currDrive);
	caseDrive[0]=currDrive[0];
	caseDrive[1]=currDrive[1];
	caseDrive[2]=currDrive[2];
	
	
	
	remoteStatus.SetPipeHandle(argv[1]);
	remoteStatus.SetState(USLATT_START);
	
	remoteStatus.WriteToPipe();
	
	StartUSLATT(&errCode);

	remoteStatus.SetState(USLATT_FINISH);
	
	remoteStatus.WriteToPipe();
}

#endif

//The use of state_with_mem.txt is clumsy but saves on ci
int maxScansExceeded()
{

	int numberOfscans=0;
	FILE * fd = fopen("state.txt","r");
	if(fd == NULL)
	{
		//Try with state_with_mem.txt
		fd = fopen("state_with_mem.txt","r");
		if(fd == NULL)
		{
			return 0;
		}
	}
	fscanf(fd,"%d",&numberOfscans);
	fclose(fd);
	if(numberOfscans <= 0)
	{
		return 0;
	}
	fd = fopen("state.txt","w");
	fprintf(fd,"%d",numberOfscans-1);
	fclose(fd);
	return numberOfscans;

}


//replace returns with setting error code
DWORD WINAPI StartUSLATT(void *errCode)
{
	gui.isTriageInProgress=true;//We have started triage and don't accept device insertion reomval notifications.
	DWORD *exitCode = (DWORD *)(errCode);
	
#ifndef TESTING
	gui.DisplayState(_T("Validating Token"));
	
	if (!validateUsbSerial())
#ifdef _DEBUG
	{
		printf("Serial Number validation failed!\n");
		AppLog.Write("Validation failed.");
	}
#else
	{
		AppLog.Write("Validation failed",WSLMSG_ERROR);
		gui.DisplayError(_T("Token validation failed, cannot proceed with the triage"));
		gui.DisplayResult(_T("Validation Failed"));
		gui.EnableErrorButton();
		*exitCode = ERR_CRITICAL_VALIDATION;
		gui.EnableEjectButton();
		gui.DisableAbortButton();
		return ERR_CRITICAL_VALIDATION; // TODO remove comment
	}
	
#endif
	
	gui.DisplayResult(_T("Validation Successful"));

	//Check if the maximum nunmber of scans has exceeded
	gui.DisplayState(_T("Checking Triage Count"));
	int scanCount=0;
	scanCount=maxScansExceeded();
	if(!scanCount)
	{
		gui.DisplayError(_T("Token has exceeded maximum number of scans. Reconfigure the token and try again."));
		gui.DisplayResult(_T("Triage Count Exceeded"));
		gui.EnableEjectButton();
		gui.DisableAbortButton();
		gui.EnableErrorButton();
		return ERR_MAXSCANS_EXCEEDED; // TODO remove comment
	}
	memset(gui.updateMsg,0,sizeof(TCHAR)*MSG_SIZE);
	_stprintf(gui.updateMsg,_TEXT("Number of scans left on the device %d"),scanCount-1); 
	gui.DisplayStatus(gui.updateMsg);
	gui.DisplayStatus(_T("Proceeding with triage ..."));
#endif

	/*MessageBox(NULL,caseDrive,_T("Drive"),MB_OK);
	exit(0);
*/
	
	stringstream ss;
    WSTRemote *w = NULL;

    // this is needed everywhere
	memset(&g_osvex, 0, sizeof(OSVERSIONINFOEX));
	g_osvex.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	INC_SYS_CALL_COUNT(GetVersionEx); // Needed to be INC TKH
	GetVersionEx((LPOSVERSIONINFO)&g_osvex);

	try
	{
		// first setup the case directory.
#ifdef _DEBUG
		TCHAR curDir[MAX_PATH];
		memset(curDir,0,MAX_PATH);
		INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
		GetCurrentDirectory(MAX_PATH,curDir);
		wcout<<"Current Directory : "<<curDir<<endl;
#endif
		w = new WSTRemote(TEXT("scanconfig.xml"));

		
		if (w != NULL) 
		{
			/*RJM*/
			AppLog.Write("ScanConfig.xml successfully read", WSLMSG_STATUS);
			gui.DisplayStatus(_T("Reading triage options ..."));
#ifndef STRIKE
			AppLog.Write("Running US-LATT PRO 2.1.0", WSLMSG_STATUS);			
#else
			AppLog.Write("Running Strike Live 1.6.0.1", WSLMSG_STATUS);			
#endif

			if(!w->SetupCaseDirectory())
			{
				AppLog.Write("Failed to create case directory",WSLMSG_ERROR);
				gui.DisplayError(_T("Failed to create case directory, cannot complete triage"));
				return ERR_CRITICAL_SETUPCASEDIRECTORY;
			}
			
			//check return value from DoInvestigation TODO
			w->DoInvestigation();//Proceed with triage after setting up case directories
		}
	}
	catch (std::exception e) 
	{
		//Remove this crap TODO
		tfstream f("wstfail.txt");
		f << "exception: " << e.what();
		f.close();
#ifdef _DEBUG
		AppLog.Write("Error creating WSTRemote", WSLMSG_DEBUG);
		AppLog.Write(e.what());
#endif
		return ERR_CRITICAL_UNKNOWN;
	}
	catch (...) 
	{
#ifdef _DEBUG
		AppLog.Write("Generic error caught in Main.cpp", WSLMSG_DEBUG);
#endif
		return ERR_CRITICAL_UNKNOWN;
	}

	cout << "Deleting wstremote object" << endl;
	delete w; 

	AppLog.Close(); //Close the log file


	XMLPostProcess(_T("\\Forensic\\AuditReport_incomplete.xml"),_T("\\Forensic\\AuditReport_purify.xml"),_T("\\Forensic\\AuditReport.xml"));
	WriteHashes();//The default arguments are NULL and the function closes the root tag with this call.

	TCHAR dirName[MAX_PATH];
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH,dirName);
#ifdef PRO
	gui.DisplayState (_T("Setting up Analysis Files"));
	gui.DisplayState (_T("Copying timeline components"));
	if(!CopyFile( (tstring(uslattDrive)+_T("Program\\timeline.zip")).c_str(),_T("Analysis\\timeline.zip"),FALSE))
	{
		gui.DisplayWarning(_T("Failed to copy timeline component from US-LATT token"));
	}

	gui.DisplayState (_T("Copying analysis functions"));
	/*if(!CopyFile( (tstring(uslattDrive)+_T("Program\\analysisFunctions.exe")).c_str(),_T("Analysis\\analysisFunctions.exe"),FALSE))
	{
		gui.DisplayWarning(_T("Failed to copy analysis functions from US-LATT token"));
	}
*/
	gui.DisplayState (_T("Copying unzip program"));
	if(!CopyFile( (tstring(uslattDrive)+_T("Program\\unzip.exe")).c_str(),_T("Analysis\\unzip.exe"),FALSE))
	{
		gui.DisplayWarning(_T("Failed to copy unzip from US-LATT token"));
	}

	gui.DisplayState (_T("Copying XML UTF-8 sanitizer program"));
	if(!CopyFile( (tstring(uslattDrive)+_T("Program\\xml-utf8-sanitizer.exe")).c_str(),_T("Analysis\\xml-utf8-sanitizer.exe"),FALSE))
	{
		gui.DisplayWarning(_T("Failed to copy XML UTF-8 from US-LATT token"));
	}

#endif
	//ReportStatus("Triage complete, performing safe unmount");
	if(!gui.Errors())
	{
		
		gui.DisplayState (_T("Triage Complete"));
		gui.DisplayResult(_T("Click \"Eject Token\""));
		if(gui.Warnings())
		{
			gui.EnableWarningButton();//At this point there are warnings but no errors
			gui.DisplayResult (_T("Click \"View Warnings\""));
			gui.DisplayStatus(_T("Triage Complete with warnings"));
		}
		else
		{
			gui.DisplayStatus(_T("Triage Complete with no errors or warnings"));
		}
	}
	else
	{
		gui.DisplayStatus(_T("Triage Complete with errors"));
		gui.DisplayState (_T("Triage Complete"));
		gui.DisplayResult (_T("Click \"View Errors\""));
		gui.EnableErrorButton();

		if(gui.Warnings())
		{
			gui.EnableWarningButton();//At this point there are both warnings and errors
		}
	}

#ifndef PRO
	//Let the investigator know that the triage is complete
	int triageIndex=0;
	int searchIndex=0;
	if( (triageIndex = SendMessage(gui.statusMsgLB,LB_FINDSTRING,searchIndex,(LPARAM)(_TEXT("Triage Complete")))) != LB_ERR)
	{
		SendMessage(gui.statusMsgLB,LB_SETCURSEL,triageIndex,NULL);
	}
	HWND hdetailButton=GetDlgItem(gui.DialogHwnd,IDC_BTN_DETAILS);
	SetWindowText(hdetailButton,TEXT("<<Hide Details"));

	SetWindowPos(gui.DialogHwnd,HWND_TOP,0, 0,g_metrics.ScaleX(618),g_metrics.ScaleY(400),SWP_NOZORDER|SWP_NOMOVE|SWP_SHOWWINDOW);

	UpdateWindow(gui.DialogHwnd);*/
	gui.detail=false;
	//Let the investigator know that the triage is complete
#endif

	gui.EnableEjectButton();
	gui.DisableAbortButton();
	//EnableWindow(hejectButton,TRUE);

	

	return ERROR_SUCCESS;
}


#undef _UNICODE
#undef UNICODE