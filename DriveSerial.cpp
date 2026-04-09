#include "stdafx.h"
#include "DriveSerial.h"

#ifndef _WIN32_WINNT
#define _WIN32_WINNT	0x0500	// Windows XP or higher - must ensure this is defined or DIRECT passthroughs won't work.
#endif

#include "ntddscsi.h"			// ^ WinNT SCSI/ATA Access
#include "winioctl.h"
#include "SysCallsLog.h" // Needed to be there to be able to INC
int GetDriveSerialSCSI(int DiskNum, unsigned char *pData, unsigned int *pDataLength)
{
	HANDLE hFile = NULL;

	SCSI_PASS_THROUGH_DIRECT sptd;
	SCSICDB	scsiCmd;
	ATAUSNVPD ataSerial;
	ATAIDENTIFYVPD ataInfo;
	unsigned char buffer[512];
	char cszPath[MAX_PATH] = "";

	DWORD ioCtlLen		= 0;		// IOCTL Data In Len
	DWORD ioCtlRet		= 0;		// IOCTL Data Out Len
	DWORD ioCtlStatus	= 0;		// IOCTL Return Code

	sprintf_s(cszPath,  MAX_PATH, "\\\\.\\PhysicalDrive%d", DiskNum);

	INC_SYS_CALL_COUNT(CreateFile); // Needed to be INC TKH
	hFile = CreateFileA(cszPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);

	if(hFile == INVALID_HANDLE_VALUE)
	{
		return 0;
	}

	// Send the SCSI commands

	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	SecureZeroMemory(&sptd, sizeof(SCSI_PASS_THROUGH_DIRECT));
	SecureZeroMemory(&scsiCmd, sizeof(SCSICDB));
	SecureZeroMemory(&ataSerial, sizeof(ATAUSNVPD));
	SecureZeroMemory(&ataInfo, sizeof(ATAIDENTIFYVPD));
	SecureZeroMemory(buffer, sizeof(unsigned char) * 512);

	scsiCmd.ucCommand	= 0x12;					// Inquiry command
	scsiCmd.ucLun		= 0x01;					// Get VPD data
	scsiCmd.ucPageCode	= 0x89;					// Get the Unit Serial Number
	scsiCmd.ucLength	= sizeof(SCSICDB);
	scsiCmd.ucControl	= 0x00;					// Not sure what this byte does

	// Copy the SCSI Command block to the SCSI Passthrough structure
	memcpy(sptd.Cdb, &scsiCmd, sizeof(SCSICDB));
	sptd.CdbLength			= sizeof(SCSICDB);
//	sptd.Length				= sizeof(SCSI_PASS_THROUGH_DIRECT) + sizeof(ATAUSNVPD);
	sptd.DataIn				= SCSI_IOCTL_DATA_OUT;
//	sptd.DataBuffer			= &ataSerial;
//	sptd.DataTransferLength = sizeof(ATAUSNVPD);
	sptd.SenseInfoLength	= 0;				// I don't think I care about SENSEINFO for this command
	sptd.SenseInfoOffset	= 0;				// ^
	sptd.PathId				= 0;				// Target SCSI Port
	sptd.Lun				= 0;				// Target LUN
	sptd.TargetId			= 1;				// Target Controller

	sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT) + 512;
	sptd.DataBuffer = buffer;
	sptd.DataTransferLength = 512;

	INC_SYS_CALL_COUNT(DeviceIoControl); // Needed to be INC TKH
	ioCtlStatus = DeviceIoControl(hFile, IOCTL_SCSI_PASS_THROUGH_DIRECT, &sptd, sizeof(SCSI_PASS_THROUGH_DIRECT), &sptd, sizeof(ATAUSNVPD), &ioCtlRet, NULL);
	
#ifdef _DEBUG	// Debugging output
	char temp1[21];
	char temp2[21];

	strncpy_s(temp1, (const char*)ataSerial.ucSerialNumber, 20);
	strcat_s(temp1, "\0");

	strncpy_s(temp2, (const char*)ataSerial.ucUnknown, 20);
	strcat_s(temp2, "\0");

	for(int i = 0; i < 20; i++)
		if(temp1[i] == '\0')
			temp1[i] = ' ';

	for(int i = 0; i < 20; i++)
		if(temp2[i] == '\0')
			temp2[i] = ' ';

	printf("IOCTL returned %d bytes\n", ioCtlRet);
	printf("Drive Serial Number: %s\n", (buffer + 8));
#endif

	INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
	CloseHandle(hFile);

	return ioCtlStatus;
}

// Get the serial number from an *ATA drive if available
int GetDriveSerialATA(int DiskNum, ATASERIAL *pAtaSerial)
{
	HANDLE hFile = NULL;

	ATA_PASS_THROUGH_DIRECT ataPTD;
	ATA_PASS_THROUGH_DIRECT ataOUT;
	ATASERIAL ataSerial;
	unsigned char ataIDBlock[512];
	char cszPath[MAX_PATH] = "";

	DWORD ioCtlStatus = 0;
	DWORD ioCtlRet	  = 0;

	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	SecureZeroMemory(&ataPTD, sizeof(ATA_PASS_THROUGH_DIRECT));
	SecureZeroMemory(&ataOUT, sizeof(ATA_PASS_THROUGH_DIRECT));
	SecureZeroMemory(&ataSerial, sizeof(ATASERIAL));
	SecureZeroMemory(ataIDBlock, sizeof(unsigned char) * 512);

	// Build the path string
	sprintf_s(cszPath,  MAX_PATH, "\\\\.\\PhysicalDrive%d", DiskNum);

	// Open the drive
	INC_SYS_CALL_COUNT(CreateFile); // Needed to be INC TKH
	hFile = CreateFileA(cszPath, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

	if(hFile == INVALID_HANDLE_VALUE)
	{
		// GetLastError() "should" provide the error that CreateFileA returned
		return 0;
	}

	ataPTD.Length				= sizeof(ATA_PASS_THROUGH_DIRECT);				// Always the length of the struct
	ataPTD.DataBuffer			= ataIDBlock;									// Pointer to our DMA buffer
	ataPTD.DataTransferLength	= sizeof(unsigned char) * 512;					// Size of our data buffer
	ataPTD.PathId				= 0;											// Filled in by the driver
	ataPTD.Lun					= 0;											// ^
	ataPTD.TargetId				= 1;											// ^
	ataPTD.AtaFlags				= ATA_FLAGS_DRDY_REQUIRED | ATA_FLAGS_DATA_IN;	// Wait for the drive to be ready
	ataPTD.TimeOutValue			= 20;											// Allow 20 Seconds for the drive to respond
	ataPTD.CurrentTaskFile[6]	= 0xEC;											// ATA_IDENTIFY command

	// Send the ATA command to the drive referenced by hFile
	INC_SYS_CALL_COUNT(DeviceIoControl); // Needed to be INC TKH
	ioCtlStatus = DeviceIoControl(hFile, IOCTL_ATA_PASS_THROUGH_DIRECT, &ataPTD, sizeof(ATA_PASS_THROUGH_DIRECT), &ataOUT, sizeof(ATA_PASS_THROUGH_DIRECT), &ioCtlRet, NULL);
	
	if(!ioCtlStatus)
	{
		// Cleanup the open handle, GetLastError() should provide whatever error DeviceIoControl() returned
		INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
		CloseHandle(hFile);
		return 0;
	}

	// Copy the data from the appropriate areas in the ATA_IDENTIFY packet
	// Serial Number is at WORD 10-19	(20 Bytes)
	// Firmware Rev is at WORD 23-26	(8 Bytes)
	// Model Number is at WORD 27-46	(40 Bytes)
	// The spec states that any nulls are converted to ASCII spaces
	// The strings are byteswapped, and NOT null terminated, hence
	// the subsequent operations.
	memcpy(ataSerial.cszSerialNumber, &ataIDBlock[20], 20);
	memcpy(ataSerial.cszFirmwareRev, &ataIDBlock[46], 8);
	memcpy(ataSerial.cszModelNumber, &ataIDBlock[54], 40);

	// Swap the bytes of each string (ATA spec)
	SwapBytes((char*)ataSerial.cszSerialNumber, 20);
	SwapBytes((char*)ataSerial.cszFirmwareRev, 8);
	SwapBytes((char*)ataSerial.cszModelNumber, 40);

	// Add null terminations to each string.
	ataSerial.cszSerialNumber[20] = '\0';
	ataSerial.cszFirmwareRev[8]	  = '\0';
	ataSerial.cszModelNumber[40]  = '\0';

	// Copy the data back to the provided buffer, if there is no buffer provided
	// let the caller know by the return code.
	if(pAtaSerial != NULL)
	{
		memcpy(pAtaSerial, &ataSerial, sizeof(ATASERIAL));
	}
	else
	{
		ioCtlStatus = 0;
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
	}

#ifdef _DEBUG	// Debugging output
	printf("ATA Serial Number: %s\n", ataSerial.cszSerialNumber);
	printf("ATA Firmware Rev: %s\n", ataSerial.cszFirmwareRev);
	printf("ATA Model Number: %s\n", ataSerial.cszModelNumber);
#endif

	INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
	CloseHandle(hFile);

	return ioCtlStatus;
}

int GetDriveSerialATAFromHandle(HANDLE hDrive, ATASERIAL *pAtaSerial)
{
	ATA_PASS_THROUGH_DIRECT ataPTD;
	ATA_PASS_THROUGH_DIRECT ataOUT;
	ATASERIAL ataSerial;
	unsigned char ataIDBlock[512];
    int iStartPos, iEndPos;

	DWORD ioCtlStatus = 0;
	DWORD ioCtlRet	  = 0;

	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	INC_SYS_CALL_COUNT(SecureZeroMemory); // Needed to be INC TKH
	SecureZeroMemory(&ataPTD, sizeof(ATA_PASS_THROUGH_DIRECT));
	SecureZeroMemory(&ataOUT, sizeof(ATA_PASS_THROUGH_DIRECT));
	SecureZeroMemory(&ataSerial, sizeof(ATASERIAL));
	SecureZeroMemory(ataIDBlock, sizeof(unsigned char) * 512);


	ataPTD.Length				= sizeof(ATA_PASS_THROUGH_DIRECT);				// Always the length of the struct
	ataPTD.DataBuffer			= ataIDBlock;									// Pointer to our DMA buffer
	ataPTD.DataTransferLength	= sizeof(unsigned char) * 512;					// Size of our data buffer
	ataPTD.PathId				= 0;											// Filled in by the driver
	ataPTD.Lun					= 0;											// ^
	ataPTD.TargetId				= 1;											// ^
	ataPTD.AtaFlags				= ATA_FLAGS_DRDY_REQUIRED | ATA_FLAGS_DATA_IN;	// Wait for the drive to be ready
	ataPTD.TimeOutValue			= 20;											// Allow 20 Seconds for the drive to respond
	ataPTD.CurrentTaskFile[6]	= 0xEC;											// ATA_IDENTIFY command

	// Send the ATA command to the drive referenced by hFile
	INC_SYS_CALL_COUNT(DeviceIoControl); // Needed to be INC TKH
	ioCtlStatus = DeviceIoControl(hDrive, IOCTL_ATA_PASS_THROUGH_DIRECT, &ataPTD, sizeof(ATA_PASS_THROUGH_DIRECT), &ataOUT, sizeof(ATA_PASS_THROUGH_DIRECT), &ioCtlRet, NULL);
	if(!ioCtlStatus)
	{
		return 0;
	}

	// Copy the data from the appropriate areas in the ATA_IDENTIFY packet
	// Serial Number is at WORD 10-19	(20 Bytes)
	// Firmware Rev is at WORD 23-26	(8 Bytes)
	// Model Number is at WORD 27-46	(40 Bytes)
	// The spec states that any nulls are converted to ASCII spaces
	// The strings are byteswapped, and NOT null terminated, hence
	// the subsequent operations.
	memcpy(ataSerial.cszSerialNumber, &ataIDBlock[20], 20);
	memcpy(ataSerial.cszFirmwareRev, &ataIDBlock[46], 8);
	memcpy(ataSerial.cszModelNumber, &ataIDBlock[54], 40);

	// Swap the bytes of each string (ATA spec)
	SwapBytes((char*)ataSerial.cszSerialNumber, 20);
	SwapBytes((char*)ataSerial.cszFirmwareRev, 8);
	SwapBytes((char*)ataSerial.cszModelNumber, 40);

	// Add null terminations to each string.
	ataSerial.cszSerialNumber[20] = '\0';
    iStartPos = 0;
    iEndPos = 19;
    while (ataSerial.cszSerialNumber[iStartPos] == ' ' && iStartPos < 20)
    {
        iStartPos++;
    }
    while (ataSerial.cszSerialNumber[iEndPos] == ' ' && iEndPos > 0)
    {
        iEndPos--;
    }
    if (iEndPos - iStartPos > 0)
    {
    	memcpy(ataSerial.cszSerialNumber, &ataSerial.cszSerialNumber[iStartPos], iEndPos - iStartPos);
        ataSerial.cszSerialNumber[iEndPos - iStartPos] = '\0';
    }
    else
    {
        ataSerial.cszSerialNumber[0] = '\0';
    }


	ataSerial.cszFirmwareRev[8]	  = '\0';
    iStartPos = 0;
    iEndPos = 7;
    while (ataSerial.cszFirmwareRev[iStartPos] == ' ' && iStartPos < 8)
    {
        iStartPos++;
    }
    while (ataSerial.cszFirmwareRev[iEndPos] == ' ' && iEndPos > 0)
    {
        iEndPos--;
    }
    if (iEndPos - iStartPos > 0)
    {
    	memcpy(ataSerial.cszFirmwareRev, &ataSerial.cszFirmwareRev[iStartPos], iEndPos - iStartPos);
        ataSerial.cszFirmwareRev[iEndPos - iStartPos] = '\0';
    }
    else
    {
        ataSerial.cszFirmwareRev[0] = '\0';
    }

	ataSerial.cszModelNumber[40]  = '\0';
    iStartPos = 0;
    iEndPos = 39;
    while (ataSerial.cszModelNumber[iStartPos] == ' ' && iStartPos < 40)
    {
        iStartPos++;
    }
    while (ataSerial.cszModelNumber[iEndPos] == ' ' && iEndPos > 0)
    {
        iEndPos--;
    }
    if (iEndPos - iStartPos > 0)
    {
    	memcpy(ataSerial.cszModelNumber, &ataSerial.cszModelNumber[iStartPos], iEndPos - iStartPos);
        ataSerial.cszModelNumber[iEndPos - iStartPos] = '\0';
    }
    else
    {
        ataSerial.cszModelNumber[0] = '\0';
    }

	// Copy the data back to the provided buffer, if there is no buffer provided
	// let the caller know by the return code.
	if(pAtaSerial != NULL)
	{
		memcpy(pAtaSerial, &ataSerial, sizeof(ATASERIAL));
	}
	else
	{
		ioCtlStatus = 0;
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
	}
	return ioCtlStatus;
}

// Swap each byte in a string of bytes
int SwapBytes(char * pChar, int Len)
{
	int		InternalLen = 0;
	int		i			= 0;
	char	Swap		= 0;

	if(!pChar || Len <= 0)
		return 0;

	if(Len % 2)
		InternalLen = Len - 1;
	else
		InternalLen = Len ;

	for(i = 0; i < InternalLen; i += 2)
	{
		Swap = pChar[i];
		pChar[i] = pChar[i+1];
		pChar[i+1] = Swap;
	}

	return i;
}