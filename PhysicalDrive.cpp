// #define _WIN32_WINNT 0x500

#include "stdafx.h"
#include "PhysicalDrive.h"
#include "windows.h"
#include "tchar.h"
#include "winioctl.h"

PhysicalDrive::PhysicalDrive(void)
{
}

PhysicalDrive::~PhysicalDrive(void)
{
}

PhysicalDrive::PhysicalDrive(int Drive)
{
	DriveNum = Drive;

	GetDriveGeo();
}

void PhysicalDrive::GetDriveGeo(void)
{
	DISK_GEOMETRY_EX* pDiskGeoEx			  = NULL;	// Disk Geometry
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
	
	_stprintf_s(DrivePath, MAX_PATH, TEXT("\\\\.\\PhysicalDrive%d"), DriveNum);

	DiskHandle = CreateFile(DrivePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);

	if(DiskHandle == INVALID_HANDLE_VALUE)
	{
		throw(GetLastError());
	}

	DGE_Size = sizeof(DISK_GEOMETRY_EX) + sizeof(DISK_PARTITION_INFO) + sizeof(DISK_DETECTION_INFO);
	pDiskGeoEx = (DISK_GEOMETRY_EX*) new unsigned char[DGE_Size];

	SecureZeroMemory(pDiskGeoEx, DGE_Size);

	// Should validate that pDiskGeoEx was allocated.
	// ...

	// Get disk information
	if(DeviceIoControl(DiskHandle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, pDiskGeoEx, DGE_Size, (LPDWORD)&IOBytes, NULL))
	{
		// Drive capacity
		Capacity = pDiskGeoEx->DiskSize.QuadPart;

		// Parse out base drive geometry
		SectorSize = pDiskGeoEx->Geometry.BytesPerSector;
		Cylinders  = pDiskGeoEx->Geometry.Cylinders.QuadPart;
		Sectors    = pDiskGeoEx->Geometry.SectorsPerTrack;
		Tracks     = pDiskGeoEx->Geometry.TracksPerCylinder;

		// BIOS Disk Info
		pBIOSDiskInfo = (DISK_DETECTION_INFO*) DiskGeometryGetDetect(pDiskGeoEx);

		if(pBIOSDiskInfo->DetectionType == DetectExInt13)
		{
			PhysicalCylinders  = pBIOSDiskInfo->ExInt13.ExCylinders;
			PhysicalHeads	   = pBIOSDiskInfo->ExInt13.ExHeads;
			PhysicalSectors	   = pBIOSDiskInfo->ExInt13.ExSectorsPerDrive;
			PhysicalSPT		   = pBIOSDiskInfo->ExInt13.ExSectorsPerTrack;
			PhysicalSectorSize = (int)pBIOSDiskInfo->ExInt13.ExSectorSize;
		}
		
		if(pBIOSDiskInfo->DetectionType == DetectInt13)
		{
			PhysicalCylinders  = pBIOSDiskInfo->Int13.MaxCylinders;
			PhysicalHeads	   = pBIOSDiskInfo->Int13.MaxHeads;
			PhysicalSPT		   = pBIOSDiskInfo->Int13.SectorsPerTrack;
			PhysicalSectorSize = 512;	// Default drive sector size if 512 bytes
			PhysicalSectors	   = PhysicalCylinders * PhysicalHeads * PhysicalSPT;
		}		
	}

	// Set up a partition layout starting with 4 partitions (max for AT BIOS)

	PDL_Size = sizeof(DRIVE_LAYOUT_INFORMATION_EX) + (4 * sizeof(PARTITION_INFORMATION_EX));
	pDriveLayout = (DRIVE_LAYOUT_INFORMATION_EX*) new unsigned char[PDL_Size];
	
	SecureZeroMemory(pDriveLayout, PDL_Size);

	IOBytes = 0;

	// This really needs to be dynamically setup to requery with a larger buffer
	if(DeviceIoControl(DiskHandle, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, NULL, 0, pDriveLayout, PDL_Size, (LPDWORD)&IOBytes, NULL))
	{
		// Determine exactly how many partitions are there (MBR style)
		for(unsigned int i = 0; i < pDriveLayout->PartitionCount; i++)
		{
			if(pDriveLayout->PartitionEntry[i].Mbr.PartitionType != PARTITION_ENTRY_UNUSED)
				ActualPartition++;
		}
	
		// Load up the real partition table
		for(unsigned int i = 0; i < ActualPartition; i++)
		{
			PARTITION Part;

			SecureZeroMemory(&Part, sizeof(PARTITION));

			Part.Start  = pDriveLayout->PartitionEntry[i].StartingOffset.QuadPart;
			Part.Length = pDriveLayout->PartitionEntry[i].PartitionLength.QuadPart;
			Part.Type   = pDriveLayout->PartitionEntry[i].Mbr.PartitionType;

			PartitionTable.push_back(Part);
		}
	}

	CloseHandle(DiskHandle);

	delete [] (unsigned char*)pDriveLayout;
	delete [] (unsigned char*)pDiskGeoEx;
}

