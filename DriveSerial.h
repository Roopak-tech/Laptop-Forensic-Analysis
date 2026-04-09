/* =========================================================================
| DriveSerial.h
| WetStone Technologies 2008
|
| Module Description:
| Provides Low Level Access to the disk drives to obtain embedded firmware
| serial numbers (if present) according to the T10, T11 and T13 
| specifications.
|
| Required compliation using the Windows NTDDK
==========================================================================*/

#include "stdafx.h"

#ifndef _DRIVESERIAL_H_
#define _DRIVESERIAL_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SCSI_CDB {
	unsigned char	ucCommand;		// SCSI Command OpCode
	unsigned char	ucLun;			// HighOrder Bits are for the LUN, low bit is for CPD
	unsigned char	ucPageCode;		// VPD Page Code
	unsigned char	ucReserved;
	unsigned char	ucLength;
	unsigned char	ucControl;
} SCSICDB, *PSCSICDB;

// ATA Spec says the Unit Serial Number should be 20 bytes long...
// And the struct looks like this
typedef struct _ATA_USN_VPD {
	unsigned char	ucPeripheral;			// Unknown
	unsigned char	ucPageCode;				// This should be returned as 0x80h
	unsigned char	ucReserved;				// Reserved
	unsigned char	ucPageLength;			// Should be sizeof(_ATA_USN_VPD - 3), why -3?
	unsigned char	ucSerialNumber[20];		// The SCSI translation filter should automatically swap bytes for us
	unsigned char	ucUnknown[20];			// For some reason my laptop is returning 40 bytes...
} ATAUSNVPD, *PATAUSNVPD;

// This is a test ATA Identify header that should be returned by a SCSI Sense, but it's not.
typedef struct _ATA_IDENTIFY_VPD
{
	unsigned char	ucPeripheral;
	unsigned char	ucPageCode;
	unsigned char	wPageLength[2];
	unsigned char	dwReserved;
	unsigned char	ucVendorID[8];
	unsigned char	ucProductID[16];
	unsigned char	ucRevision[4];
	unsigned char	ucSignature[24];
	unsigned char	ucCommandCode;
	unsigned char	ucReserved[3];
	unsigned char	ucIdentifyData[512];
} ATAIDENTIFYVPD, *PATAIDENTIFYVPD;

// The actual ATA Serial Number format, string format, byteswapped, and null terminated.
typedef struct _ATA_SERIAL {
	char	cszSerialNumber[21];
	char	cszFirmwareRev[9];
	char	cszModelNumber[41];
} ATASERIAL, *PATASERIAL;

int GetDriveSerialSCSI(int DiskNum, unsigned char *pData, unsigned int *pDataLength);
int GetDriveSerialATA(int DiskNum, PATASERIAL pAtaSerial);
int GetDriveSerialATAFromHandle(HANDLE hDrive, ATASERIAL *pAtaSerial);
int SwapBytes(char *pChar, int Length);

#ifdef __cplusplus
}
#endif

#endif // _DRIVESERIAL_H_