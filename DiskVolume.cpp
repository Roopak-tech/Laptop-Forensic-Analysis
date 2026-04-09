#include "stdafx.h"
#include "DiskVolume.h"
#include "WSTLog.h"

DiskVolume::DiskVolume(void)
{
}

DiskVolume::~DiskVolume(void)
{
}

DiskVolume::DiskVolume(TCHAR* GUID)
{
	SetGuid(GUID);
	EnumMountPoints();
	EnumExtents();
}

int DiskVolume::SetGuid(TCHAR* GUID)
{
    unsigned int pos;
	if(GUID == NULL)
    {
		throw(TEXT("Null Mount Point"));
    }
	else
	{
	}

	return 1;
}

int DiskVolume::EnumMountPoints(void)
{
	char szPath[MAX_PATH];
	string szTempVolID;
	OSVERSIONINFO osv;
	unsigned int StrLen	   = 0;
	unsigned int DriveType = 0;
	int i = 0;
	TCHAR tempDrive[MAX_PATH] = _T("c:\\");
	TCHAR bufVol[MAX_PATH];
	HMODULE hKernel = NULL;

	lpfnGetVolumePathNamesForVolumeName GetVolumePathNamesForVolumeName = NULL;
	
	memset(&osv, 0, sizeof(OSVERSIONINFO));
	osv.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if(szVolumeGUID.length() == 0)
		return 0;

    szTempVolID = TEXT("\\\\?\\Volume");
	szTempVolID += szVolumeGUID;
	szTempVolID += "\\";
	// Many many ways to skin a cat...
	if(GetVersionEx(&osv))
	{
		if((osv.dwMajorVersion >= 5) && (osv.dwMinorVersion > 0))
		{
			hKernel = LoadLibraryEx(_T("kernel32.dll"), NULL, 0);

			if(hKernel != NULL)
			{
				GetVolumePathNamesForVolumeName = (lpfnGetVolumePathNamesForVolumeName)GetProcAddress(hKernel, GVPNFVN);
				AppLog.Write("GetVolumePathNamesForVolumeNameA API", WSLMSG_PROF);
			}
			GetVolumePathNamesForVolumeName(szTempVolID.c_str(), szPath, MAX_PATH, (DWORD*)&StrLen);
		}
		else
		{
            szPath[0] = 0;
			for(i = _T('c'); i <= _T('z'); i++)
			{
				memset(bufVol, 0, MAX_PATH * sizeof(TCHAR));

				tempDrive[0] = i;
				GetVolumeNameForVolumeMountPoint(tempDrive, bufVol, MAX_PATH);
				if(szTempVolID == bufVol)
				{
					strncpy_s(szPath, MAX_PATH, tempDrive, _TRUNCATE);
//					break;
				}
			} 			
		}
	}

	if(strlen(szPath) > 0)
	{
		MountPoints.push_back(szPath);

		// Might as well get the info on the volume while we're here.
		GetVolumeInformation(szPath, (LPSTR)szVolumeName.c_str(), MAX_PATH, NULL, NULL, NULL, (LPSTR)szFileSystem.c_str(), MAX_PATH);
		DriveType = GetDriveType(szPath);

		switch(DriveType)
		{
		case DRIVE_REMOVABLE:
			szType = "Removable";
			break;
		case DRIVE_FIXED:
			szType = "Fixed Hard Disk";
			break;
		case DRIVE_REMOTE:
			szType = "Remote Storage (eg: Network)";
			break;
		case DRIVE_CDROM:
			szType = "CD/DVD Drive";
			break;
		case DRIVE_RAMDISK:
			szType = "RAMDisk";
			break;
		case DRIVE_NO_ROOT_DIR:
			szType = "Could not identify the volume";
			break;
		default:
			szType = "Unknown Drive Type";
		}
	}

	return 1;
}

int DiskVolume::EnumExtents(void)
{
	VOLUME_DISK_EXTENTS* VolExtents = NULL;
	HANDLE VolumeHandle				= NULL;
	unsigned int IOBytes			= 0;
	int NumExtents					= 0;
	int	IOReturn					= 0;
	string szTempVolID;
	
	// All the work we did above, for nothing. :)
	// Format the Volume GUID appropriately for our use
	szTempVolID = TEXT("\\\\?\\Volume");
	szTempVolID += szVolumeGUID;
//	szTempVolID += "\\";
//	TempVolID.Format(TEXT("\\\\?\\Volume%s"), VolumeGUID);

	// Volume handles require share modes
	VolumeHandle = CreateFile(szTempVolID.c_str(), 
							  GENERIC_READ | GENERIC_WRITE,
							  FILE_SHARE_READ | FILE_SHARE_WRITE,
							  NULL,
							  OPEN_EXISTING,
							  0,
							  NULL);

	// Test to ensure we have a good handle
	if(VolumeHandle == INVALID_HANDLE_VALUE)
	{
		// Bad handle
		// throw(TEXT("Bad Volume Handle."));  // Should really come up with some sort of exception handling
		return 0;
	}

	// Get the disk extents for the volume
	VolExtents = (VOLUME_DISK_EXTENTS*) new unsigned char[sizeof(VOLUME_DISK_EXTENTS)];
	SecureZeroMemory(VolExtents, sizeof(VOLUME_DISK_EXTENTS));

	IOReturn = DeviceIoControl(VolumeHandle, 
							   IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
							   NULL, 0,
							   VolExtents, sizeof(VOLUME_DISK_EXTENTS),
							   (LPDWORD)&IOBytes,
							   NULL);
	
	if(!IOReturn && (GetLastError() & (ERROR_INSUFFICIENT_BUFFER | ERROR_MORE_DATA)))
	{
		// More than likely the device isn't supported, or it's a removable device
		// reporting that it's not ready.
		delete [] (unsigned char*)VolExtents;
		TotalDrives = 0;
		return 0;
	}

	// Buffer isn't big enough?
	if(VolExtents->NumberOfDiskExtents > 1)
	{
		VOLUME_DISK_EXTENTS* TempVol = (VOLUME_DISK_EXTENTS*) new unsigned char[sizeof(VOLUME_DISK_EXTENTS) + (VolExtents->NumberOfDiskExtents * sizeof(DISK_EXTENT))];
		SecureZeroMemory(TempVol, sizeof(VOLUME_DISK_EXTENTS) + (VolExtents->NumberOfDiskExtents * sizeof(DISK_EXTENT)));
		
		delete [] (unsigned char*)VolExtents;
		
		VolExtents = TempVol;

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
			TotalDrives = 0;
			return 0;
		}
	}

	CloseHandle(VolumeHandle);
//	TempVolID.UnlockBuffer();

	// We should have our volume extents now.
	for(unsigned int i = 0; i < VolExtents->NumberOfDiskExtents; i++)
	{
		DISKEXT DExt;
		DExt.DriveNum = VolExtents->Extents[i].DiskNumber;
		DExt.Length   = VolExtents->Extents[i].ExtentLength.QuadPart;
		DExt.Offset   = VolExtents->Extents[i].StartingOffset.QuadPart;
		DiskExtents.push_back(DExt);
	}

	TotalDrives = VolExtents->NumberOfDiskExtents;

	delete [] (unsigned char*)VolExtents;

	return 1;
}