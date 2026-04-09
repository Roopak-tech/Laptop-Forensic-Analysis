/*
 * This code runs before memory acquisition - use as little memory as is
 * reasonably possible! Because of this, there are several bad practices
 * in use here. Beware, but the comments should explain.
 */
#define _UNICODE
#define UNICODE

#include <windows.h>
#include <shlobj.h>
#include <cstdio>
#include "stdafx.h"
#include "USB Device Utilities.h"
#include "SysCallsLog.h"
#include "gui.h"

extern gui_resources gui;

// this is global for a reason, can't recall why at the moment, probably b/c we are lazy
LPTSTR logoPath;

// static shared character buffer for API calls, etc to save memory
static TCHAR shared_buffer[MAX_PATH + 1];

// The logo bitmap is supposed to be in the root directory of the USB token!
static TCHAR logo_path[] = _T("X:\\ForensicLogo.bmp");

// Prototype for the validation function in usbprotect.dll
typedef int (*ValidateProtectionFunc)(char *, char*);


bool isUSB(LPCWSTR dev_path)
{
	HANDLE h_devfile;
	STORAGE_DEVICE_DESCRIPTOR sdd;
	STORAGE_PROPERTY_QUERY query;
	DWORD nbytes_ret = 0;
	DWORD result = -1;

	query.PropertyId=StorageDeviceProperty;
	query.QueryType=PropertyStandardQuery;

	INC_SYS_CALL_COUNT(CreateFileW); // Needed to be INC TKH
	h_devfile = CreateFileW(
		dev_path, 
		0, 
		0,
		NULL, 
		OPEN_EXISTING,
		0, 0 );
	if (h_devfile != INVALID_HANDLE_VALUE)
	{
		INC_SYS_CALL_COUNT(DeviceIoControl); // Needed to be INC TKH
		if ( DeviceIoControl(h_devfile,
				IOCTL_STORAGE_QUERY_PROPERTY, 
				&query, sizeof(STORAGE_PROPERTY_QUERY), 
				&sdd, 
				sizeof(sdd), 
				&nbytes_ret, 
				NULL) )
		{
			if(sdd.BusType == BusTypeUsb)
				return true;
			else
				return false;
		}
		else
			result=GetLastError();
	}
	return false;
}


/*
 * Finds which device the logo bitmap is on, and fills in the drive letter
 * in the logo_path global buffer.
 */
static bool FindForensicLogo() {
	
	// Buffer to hold the strings identifying each logical drive in the system
	LPTSTR driveBuf = NULL;
	DWORD length1 = 0, length2 = 0;
	TCHAR drivePath[20];

	// The first call is to figure out how big to make the buffer
	INC_SYS_CALL_COUNT(GetLogicalDriveStrings); // Needed to be INC TKH
	length1 = GetLogicalDriveStrings(0, driveBuf);
	if (length1 ==0)
		return false;

	driveBuf = (LPTSTR) malloc(length1 * sizeof(TCHAR));

	// Now we should get some results back
	INC_SYS_CALL_COUNT(GetLogicalDriveStrings); // Needed to be INC TKH
	length2 = GetLogicalDriveStrings(length1, driveBuf);
	if ((length2 == 0) && (length2 > length1))
		return false;

	// Walk across the strings in the returned buffer until we get to the 
	// null terminator. Note that we skip across the null at the end of each
	// drive string by adding 1 to the pointer increment. 
	for (LPCTSTR drive = driveBuf; *drive; drive += _tcslen(drive) + 1) {

		WIN32_FIND_DATA findData;
		HANDLE hFind;
		DWORD tempType = 0;

		// We are only interested in removable devices, of course
		// Added isUSB to ensure the usb bus is detected for Iomega drives
		memset(drivePath,0,20);
		_tcsncpy(drivePath,L"\\\\.\\",7);
		_tcsncat(drivePath,drive,1);
		_tcsncat(drivePath,L":",1);

		INC_SYS_CALL_COUNT(GetDriveType); // Needed to be INC TKH
		if ((GetDriveType(drive) != DRIVE_REMOVABLE) && (isUSB(drivePath) == false))
			continue;

		// prevent error popups due to empty removable drives
		SetErrorMode(SEM_FAILCRITICALERRORS);
		INC_SYS_CALL_COUNT(GetVolumeInformation); // Needed to be INC TKH
		if (GetVolumeInformation(drive, shared_buffer, MAX_PATH + 1, NULL, NULL, NULL, NULL, 0)) {
			if (_tcscmp(shared_buffer, _T("US-LATT")) != 0 && _tcscmp(shared_buffer, _T("STRIKE")) != 0 )
				// invalid label - keep looking
				continue;
		}
		else {
			// if we couldn't get info about a volume, just move on to the next one
			continue;
		}

		logo_path[0] = drive[0];

		hFind = FindFirstFile(logo_path, &findData);
		if((hFind != INVALID_HANDLE_VALUE) && ((DWORD)hFind != ERROR_FILE_NOT_FOUND)) {
			FindClose(hFind);
			return true;
		}
	}

	return false;
}

// Open up USBProtect.dll and get ahold of the ValidateProtection function
static ValidateProtectionFunc load_protect_dll()
{
	ValidateProtectionFunc funcptr = NULL;
	INC_SYS_CALL_COUNT(LoadLibrary); // Needed to be INC TKH
	HMODULE hUsbDll = LoadLibrary(_T("USBProtect.dll"));
	if (hUsbDll == NULL)
		return NULL;
 
	INC_SYS_CALL_COUNT(GetProcAddress); // Needed to be INC TKH
	funcptr = (ValidateProtectionFunc) GetProcAddress(hUsbDll, "ValidateProtection");
	if (funcptr == NULL)
	{
		INC_SYS_CALL_COUNT(FreeLibrary); // Needed to be INC TKH
		FreeLibrary(hUsbDll);
	}
	
	return funcptr;
}

extern char serialNumber[];
bool validateUsbSerial() 
{
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	SecureZeroMemory(shared_buffer, MAX_PATH + 1);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	SecureZeroMemory(&si,sizeof(si)); 
	si.cb=sizeof(si);
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	SecureZeroMemory(&pi,sizeof(pi)); 
	
	ValidateProtectionFunc lpfnValidateProtectionFunc = load_protect_dll();
	if (lpfnValidateProtectionFunc == false)
	{
		gui.DisplayError(_T("USB Protect DLL failed to load"));
		return false;
	}

	// get this drive's serial number - kind of a pain since USB code works with wide chars
	size_t nchar = 0;
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH, shared_buffer);

	TCHAR my_drive[] = _T("\\\\.\\X:");
	my_drive[4]=shared_buffer[0];

#ifdef UNICODE
	char shared_bufferA[2*(MAX_PATH) + 1];	
#endif

	// save serial number for later
	FILE *fd;
	bool override_flag = false;

	LPTSTR serial_number = get_usb_storage_device_serial_number(my_drive);
	
	if(serial_number != NULL)
	{
		_tcsncpy(shared_buffer,serial_number,MAX_PATH);	
#ifdef UNICODE
		wcstombs(shared_bufferA,shared_buffer,2*MAX_PATH);
#endif
		gui.DisplayStatus(_T("Collecting license information ..."));
	}
	else
	{
		if (fopen_s(&fd, "\\.id", "r+") == 0)
		{

			fscanf(fd, "%s", shared_bufferA);
			override_flag=true;
			gui.DisplayStatus(_T("Acquiring license information ..."));
			fclose(fd);
		}
		else
		{
			gui.DisplayError(_T("Unable to obtain the license")); 
			return false;
		}
	}
	

	if ( (fopen_s(&fd, "\\.id", "w") == 0) )
	{
		
		fprintf(fd, "%s", shared_bufferA);
		fclose(fd);
	}
	

	// the path is set in logo_path by this function
	if (FindForensicLogo() == false)
	{
		gui.DisplayError(_T("Cannot find Logo file"));
		return false;
	}
#ifdef UNICODE
	char logo_pathA[MAX_PATH];
	wcstombs(logo_pathA,logo_path,MAX_PATH);
#endif


	int ret = lpfnValidateProtectionFunc(shared_bufferA, logo_pathA);
	if (ret == 0) //Watermark verified
	{
		//Store the serial number for reporting
		strncpy(serialNumber,shared_bufferA,2*(MAX_PATH));
		return true;
	}
	return false;//Watermark not verified
}

#undef _UNICODE
#undef UNICODE