

/* 
 *  Application side code for capturing physical memory on x64 bit Windows Vista/7 machines
 *  The code uses need microsoft signing for the memory capture driver
 *  The Entrypoint in this file is DumpMemory64()
 *
 */

#define UNICODE
#define _UNICODE

#include <shlobj.h>
#include "x64PhysicalMemoryCapture.h"
#include "fwmemmap.h"
#include "stddefs.h"
#include <sstream>
#include <iostream>
#include "WSTRemote.H"
#include "WSTLog.h"
#include "SysCallsLog.h"
using namespace std;

#define     ONE_MB	(1 << 20)
#define     FOUR_GB	((ULONGLONG) 1 << 32)

extern int WriteHashes(tstring filename=_T("NULL"),tstring hashValue=_T("NULL"));
memory_map *memory_region_file_map=0;


	


TCHAR msgBuf[MAX_PATH];
//extern int WriteHashes(tstring filename=_T("NULL"),tstring hashValue=_T("NULL"));

//Cleanup by closing all the open handles in case of error or as part of BAU.

void MemoryCaptureCleanUp(IN SC_HANDLE hSCM=NULL,IN SC_HANDLE hSVC=NULL,IN HANDLE hphysmemDevice = INVALID_HANDLE_VALUE)
{
	//First close the driver handle if appropriate
	if( hphysmemDevice != INVALID_HANDLE_VALUE)
	{
		INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
		CloseHandle(hphysmemDevice);
	}

	//Next stop and remove service if applicable
	if(hSVC)
	{
		SERVICE_STATUS ss;

		INC_SYS_CALL_COUNT(ControlService); // Needed to be INC TKH
		if(!ControlService(hSVC,SERVICE_CONTROL_STOP,&ss))
		{
			//AppLog.Write("Could not stop the service x64",WSLMSG_NOTE);
		}

		INC_SYS_CALL_COUNT(DeleteService); // Needed to be INC TKH
		if(!DeleteService(hSVC))
		{
			//AppLog.Write("Could not delete service x64",WSLMSG_NOTE);
		}

		//Finally close service handle
		INC_SYS_CALL_COUNT(CloseServiceHandle); // Needed to be INC TKH
		CloseServiceHandle(hSVC);
	}

	//Now close the handle to the service manager
	if(hSCM)
	{
		INC_SYS_CALL_COUNT(CloseServiceHandle); // Needed to be INC TKH
		CloseServiceHandle(hSCM);
	}
}

#ifdef PRO
extern tstring lattDrive;
#endif
//Load a driver and call it to obtain the handle to a physical memory \Device\PhysicalMemory
//Close physmemHandle after exit.
//MemoryMap changes -- RJM (03/04/2011)
int LoadDriver(OUT SC_HANDLE &hSCM,OUT SC_HANDLE &hSVC,OUT HANDLE &hphysmemDevice,IN int arch64)//32 bit arch change
{

	TCHAR driverPathName[MAX_PATH];
	
	memset(driverPathName,NULL,MAX_PATH);


	if(arch64)//32 bit arch change
	{
		//Get the full pathname for the driver from the program directory
		INC_SYS_CALL_COUNT(GetFullPathName); // Needed to be INC TKH
#ifdef PRO
		if(!GetFullPathName( (lattDrive + _T("Program\\WSTMemCap64.sys")).c_str(),MAX_PATH,driverPathName,NULL))//32 bit arch change
#else
		if(!GetFullPathName(L"..\\..\\Program\\WSTMemCap64.sys",MAX_PATH,driverPathName,NULL))//32 bit arch change
#endif
		{
			gui.DisplayError(_T("64-bit Physical Memory Capture, could not obtain pathname for the driver"));
			AppLog.Write("Could not obtain the full pathname for the driver",WSLMSG_ERROR);
			//cout<<"Could not obtain handle to the sys file"<<endl;
			return ERR_PATH_NOT_FOUND_x64;
		}
		gui.DisplayStatus(_TEXT("Loading 64-bit memory capture driver ..."));
		gui.DisplayState(_TEXT("Loading driver ..."));
		AppLog.Write("Loading 64 bit memory driver",WSLMSG_STATUS);
	}
	else//32 bit arch change
	{
		INC_SYS_CALL_COUNT(GetFullPathName); // Needed to be INC TKH
#ifdef PRO
		if(!GetFullPathName((lattDrive + _T("Program\\WSTMemCap32.sys")).c_str(),MAX_PATH,driverPathName,NULL))//32 bit arch change
#else
		if(!GetFullPathName(L"..\\..\\Program\\WSTMemCap32.sys",MAX_PATH,driverPathName,NULL))//32 bit arch change
#endif
		{
			gui.DisplayError(_T("32-bit Physical Memory Capture, could not obtain pathname for the driver"));
			AppLog.Write("Could not obtain the full pathname for the driver",WSLMSG_ERROR);
			//cout<<"Could not obtain handle to the sys file"<<endl;
			return ERR_PATH_NOT_FOUND_x64;
		}
			gui.DisplayStatus(_TEXT("Loading 32-bit memory capture driver ..."));
			gui.DisplayState(_TEXT("Loading driver ..."));
			AppLog.Write("Loading 32 bit memory driver",WSLMSG_STATUS);
	}


	//AppLog.Write(driverPathName,WSLMSG_STATUS);
	//Now we use the service manager to load the driver similar to vista capture

	//Open the SC manager
	INC_SYS_CALL_COUNT(OpenSCManager); // Needed to be INC TKH
	hSCM=OpenSCManager(
		NULL,
		NULL,
		SC_MANAGER_CREATE_SERVICE
		);

	if(!hSCM)
	{
		gui.DisplayError(_TEXT("Opening SC manager during physical memory capture"));
		AppLog.Write("Error opening SC Manager in x64 phsyical memory capture",WSLMSG_ERROR);
		return ERR_SC_OPEN_x64;
	}


	//Now create the service
	INC_SYS_CALL_COUNT(CreateService); // Needed to be INC TKH
	hSVC=CreateService(
		hSCM,
		DRIVER_NAME,
		DRIVER_DISPLAYNAME,
		SERVICE_ALL_ACCESS,
		SERVICE_KERNEL_DRIVER,
		SERVICE_DEMAND_START,
		SERVICE_ERROR_IGNORE,
		driverPathName,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
		);

	if(hSVC == NULL)
	{
		
		if(GetLastError() == ERROR_SERVICE_EXISTS)
		{
			cout<<"Service already exists"<<endl;
			INC_SYS_CALL_COUNT(OpenService); // Needed to be INC TKH
			gui.DisplayStatus(_T("Service exists, opening service"));
		    hSVC = OpenService(hSCM, L"WSTMemCap", SERVICE_ALL_ACCESS);
		}
		else
		{
			MemoryCaptureCleanUp(hSCM);
			LOG_SYSTEM_ERROR(_TEXT("Create Service failed in 64 bit physical memory capture"));
			gui.DisplayError(_TEXT("Create service failed"));
			return ERR_CREATE_SVC_x64;
		}
	}

	//Start the driver
	INC_SYS_CALL_COUNT(StartService); // Needed to be INC TKH
	if(!StartService(hSVC,0,NULL))
	{
		gui.DisplayStatus(_TEXT("Starting memory capture driver ..."));
		
		if(GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
		{
			SERVICE_STATUS ss;
			INC_SYS_CALL_COUNT(ControlService); // Needed to be INC TKH
			ControlService(hSVC,SERVICE_CONTROL_STOP,&ss);
			AppLog.Write("Stopping service in x64 memory capture",WSLMSG_STATUS);
			if(ss.dwCurrentState == SERVICE_RUNNING)
			{
				LOG_SYSTEM_ERROR(_TEXT("Failed to stop service in 64 bit physical memory capture"));
				gui.DisplayError(_TEXT("Failed to stop service in 64 bit physical memory capture"));
				MemoryCaptureCleanUp(hSCM,hSVC);
				return ERR_START_SVC_x64;
			}
		}
		else if(!StartService(hSVC,0,NULL))
		{
			INC_SYS_CALL_COUNT(StartService); // Needed to be INC TKH
			LOG_SYSTEM_ERROR(_TEXT("Create Service failed in 64 bit physical memory capture"));
			gui.DisplayError(_TEXT("Failed to start memory capture driver ..."));
			MemoryCaptureCleanUp(hSCM,hSVC);
			return ERR_START_SVC_x64;
		}
			

	}



	//Open the driver
	INC_SYS_CALL_COUNT(CreateFile); // Needed to be INC TKH
	hphysmemDevice = CreateFile(
		L"\\\\.\\WSTMemCap",
		GENERIC_ALL,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		0,
		NULL
		);

	if(hphysmemDevice == INVALID_HANDLE_VALUE)
	{
		memset(msgBuf,0,MAX_PATH);
		_sntprintf_s(msgBuf,MAX_PATH,MAX_PATH,L"Error obtaining handle for physical memory device x64 %d",GetLastError());
		AppLog.Write(msgBuf,WSLMSG_ERROR);
		MemoryCaptureCleanUp(hSCM,hSVC);
		return ERR_INVALID_HANDLE_x64;
	}

	return DRIVER_LOAD_SUCCESS;
}

int OpenHandleToPhysicalMemory(OUT PHANDLE physmemHandle,IN SC_HANDLE hSCM,IN SC_HANDLE hSVC,IN HANDLE hphysmemDevice)
{
	gui.DisplayStatus(_TEXT("Obtaining handle to physical memory ..."));
	DWORD bytesReturned=0;

	//Send open control code through DeviceIoControl to obtain the handle to the memory capture device. 
	SetLastError(0);
	INC_SYS_CALL_COUNT(DeviceIoControl); // Needed to be INC TKH
	if(!DeviceIoControl(
		hphysmemDevice,
		GET_MEMORY_HANDLE,
		NULL,
		0,
		physmemHandle,
		sizeof(HANDLE),
		&bytesReturned,
		NULL
		))
	{
		//AppLog.Write("Error in DeviceIoControl in physical memory device x64",WSLMSG_ERROR);
		
		cout<<"Error in DeviceIoControl in physical memory device x64"<<endl;
		gui.DisplayError(_TEXT("Could not obtain handle to physical memory device"));
		cout<<GetLastError()<<endl;
		MemoryCaptureCleanUp(hSCM,hSVC,hphysmemDevice);
		return ERR_DIO_CTL_x64;
	}

	//MemoryCaptureCleanUp(hSCM,hSVC,hphysmemDevice);
	//AppLog.Write("Obtained Handle to the physical memory device",WSLMSG_STATUS);
	return FILE_OPEN_SUCCESS_x64;
}

DWORD GetMap (HANDLE Device, PE820_MAP *Map,SC_HANDLE hSCM,HANDLE hSVC)
{
	gui.DisplayStatus(_TEXT("Analyzing OS memory regions ..."));
	PE820_MAP map;
	DWORD ec = ERROR_SUCCESS;

	for (DWORD cb = 0x0100; ec == ERROR_SUCCESS; delete map) 
	{

		map = (PE820_MAP) new CHAR [cb];

		if (map == NULL)
		{
			ec = ERROR_NOT_ENOUGH_MEMORY;
			break;
		}

		DWORD cbret;

		INC_SYS_CALL_COUNT(DeviceIoControl); // Needed to be INC TKH
		BOOL result = DeviceIoControl (Device, IOCTL_FWMEMMAP_GET_MAP,NULL, 0, map, cb, &cbret, NULL);

		if (result)
		{
			*Map = map;
			return ERROR_SUCCESS;
		}

		ec = GetLastError ();

		if (ec == ERROR_MORE_DATA AND cbret >= sizeof (map -> Count))
		{
			cout<<"Allocating more space for E820 MAP"<<endl;
			cb = FIELD_OFFSET (E820_MAP, Descriptors) + map -> Count * sizeof (E820_DESCRIPTOR); //Allocate the appropriate amount
			ec = ERROR_SUCCESS;
		}
	}

	return ec;
}

// Return TRUE if there is enough space to create write spaceRequired to
// the file pointed by handle *fh.
#ifdef PRO
extern TCHAR caseDrive[];
#endif
BOOL isSpaceEnough(IN PHANDLE fh,IN DWORDLONG spaceRequired)
{
	ULARGE_INTEGER li;

	li.QuadPart = spaceRequired;

	if(spaceRequired <= 0)
		return TRUE;

	//use the logic below -- RJM 02/21/2011
	ULARGE_INTEGER la;
	INC_SYS_CALL_COUNT(GetDiskFreeSpaceEx); // Needed to be INC TKH
#ifdef PRO
	GetDiskFreeSpaceEx(caseDrive,NULL,NULL,&la);
#else
	GetDiskFreeSpaceEx(NULL,NULL,NULL,&la);
#endif

	if(li.QuadPart > la.QuadPart)
	{
		gui.DisplayError(_TEXT("Not enough space to write the physical memory file"));
		AppLog.Write("Not enough space to write physmem.bin file x64",WSLMSG_ERROR);
		return FALSE;
	}
	return true;
}


//Cleanup on exit from DumpMemory64

void CleanUpOnExitFromDumpMemory64(IN HANDLE hBin=NULL,IN SC_HANDLE hSCM=NULL,IN SC_HANDLE hSVC=NULL,IN HANDLE hphysmemDevice = INVALID_HANDLE_VALUE)
{

	MemoryCaptureCleanUp(hSCM,hSVC,hphysmemDevice);
	gui.DisplayStatus(_TEXT("Performing physical memory resource cleanup ..."));
	AppLog.Write("Closing physical memory handle after physical memory acquisition",WSLMSG_STATUS);
	if(hBin && (hBin != INVALID_HANDLE_VALUE))
	{
		INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
		CloseHandle(hBin);
	}
}

//Free all the memory resources used 
void FreeResources(BYTE *chunkBuf,BYTE *nullPage,chunk_count_struct *chunk_count_array)
{
	
	if(chunkBuf)
		delete[] chunkBuf;
	AppLog.Write("Releasing chunkBuf",WSLMSG_STATUS);
	
	if(nullPage)
		delete[] nullPage;
	AppLog.Write("Releasing nullPage",WSLMSG_STATUS);

	if(chunk_count_array)
		delete[] chunk_count_array;
	AppLog.Write("Releasing chunk_count_array",WSLMSG_STATUS);
}

//02/18/2011 RJM -- Multiple memory file capture change

HANDLE CreatePhysmemFile(OUT const TCHAR *fileName)
{
	static int counter=1;
	HANDLE hBin;
	wostringstream physFileNameW;

	memset(const_cast<TCHAR *>(fileName),0,MAX_PATH);
	physFileNameW<<PHYSMEMFILE_NAME<<counter++<<L".bin";

	wcscpy_s(const_cast<TCHAR *>(fileName),MAX_PATH,physFileNameW.str().c_str());
	//_tcsncpy(const_cast<TCHAR *>(fileName),physFileNameW.str().c_str(),physFileNameW.str().length());
	//First open the file that will contain the raw dump
	INC_SYS_CALL_COUNT(CreateFile); // Needed to be INC TKH
	hBin=CreateFile(
		physFileNameW.str().c_str(),
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		0,
		NULL
		);

	return hBin;
}

int GetOsMemoryRanges (PE820_MAP Map,os_memory_range ** physical_memory_accesible)
{
	ULONGLONG total = 0;
	ULONGLONG above4GB = 0;

	int memory_count=0;
	DWORD n=0;

	LARGE_INTEGER start;
	LARGE_INTEGER size;
	LARGE_INTEGER end;

	memory_count = Map -> Count;//Changed to display all the memory regions
	/*
	for (n = 0; n < Map -> Count; n ++)
	{
		if(Map -> Descriptors [n].Type == 1)
		{
			memory_count++;
		}
	}
	*/

	//*physical_memory_accesible= (os_memory_range *)malloc(sizeof(os_memory_range)*memory_count);
	*physical_memory_accesible = new os_memory_range[memory_count];
	memory_count=0;//For reuse in assigning values
	
	for (n = 0; n < Map -> Count; n ++) 
	{

		PCSTR type;

		switch (Map -> Descriptors [n].Type)
		{
		case 1: 
			{
			type = "memory";
			break;
			}
		case 2:
			{
			type = "reserved";
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
			type = "undefined";
			break;
			 }
		}
		/*
		printf ("0x%08X`%08X 0x%08X`%08X  %d (%s)\n",
		Map -> Descriptors [n].Base.HighPart,
		Map -> Descriptors [n].Base.LowPart,
		Map -> Descriptors [n].Size.HighPart,
		Map -> Descriptors [n].Size.LowPart,
		Map -> Descriptors [n].Type,
		type);
		*/
		if (Map -> Descriptors [n].Type == 1)
		{
			start= Map -> Descriptors [n].Base;
			size= Map -> Descriptors [n].Size;
			end.QuadPart = start.QuadPart + size.QuadPart;

			((*physical_memory_accesible)[memory_count]).start.HighPart = start.HighPart;
			((*physical_memory_accesible)[memory_count]).start.LowPart = start.LowPart;
			((*physical_memory_accesible)[memory_count]).end.HighPart = end.HighPart;
			((*physical_memory_accesible)[memory_count]).end.LowPart = end.LowPart;

			//Added to capture more information RJM
			((*physical_memory_accesible)[memory_count]).size=size;
			((*physical_memory_accesible)[memory_count]).type=1;
		
			memory_count+=1;


			if (end.QuadPart > FOUR_GB) 
			{
				above4GB += (end.QuadPart - max (start.QuadPart, FOUR_GB));
			}
			total += size.QuadPart;

		}
		else
		{
			//Added to capture more information RJM
			((*physical_memory_accesible)[memory_count]).start=Map ->Descriptors[n].Base;
			((*physical_memory_accesible)[memory_count]).size=Map ->Descriptors[n].Size;
			((*physical_memory_accesible)[memory_count]).type=Map ->Descriptors[n].Type;
			((*physical_memory_accesible)[memory_count]).end.QuadPart=Map ->Descriptors[n].Base.QuadPart+Map ->Descriptors[n].Size.QuadPart;
			memory_count+=1;
		}




	}
	return memory_count;
}

VOID ReleaseMap (PE820_MAP Map)
{
    delete (PCHAR) Map;
}


//02/18/2011 RJM -- Multiple memory file capture change
void writeHashToDigest(IN TCHAR fileName[] ,IN MD5_CTX md5Data,IN FileHasher m_hasher)
{
	AppLog.Write("Writing physical memory hash to file",WSLMSG_STATUS);
	//Finalize hash and write it out to the digest file
	unsigned char physMemHash[20];
	memset(physMemHash,0,20);
	MD5Final(physMemHash,&md5Data);
	//Write the hash to the digest file
	tstring hash;
	hash=m_hasher.MD5BytesToString(physMemHash);
	tstring pathName=L"\\"+tstring(fileName);	
	WriteHashes(pathName.c_str(),hash);
}


int compute_chunks(IN int PAGE_SIZE,IN LARGE_INTEGER start, IN LARGE_INTEGER end,OUT ULONGLONG &partial)
{

	int number_of_chunks=0;

	number_of_chunks = (end.QuadPart - start.QuadPart) / (PAGE_SIZE * SPEEDUP);
	partial = (end.QuadPart - start.QuadPart) % (PAGE_SIZE * SPEEDUP);
	return number_of_chunks;
}


//We need a null page of exactly the size of the view  that we tried to map. This could be problem because
//some part of the view might be accessible. This can be resolved by reducing the view size to that of a a PAGE
//However this will reduce the speed of capture drastically. All this implies that currently the physical memory
//capture might not contain all the "physical pages" of the live system. 

int GrabChunkFromPhysicalMemory(IN ULONGLONG offset,IN ULONG chunk,IN BYTE *chunkBuf,IN BYTE *nullPage,IN HANDLE physmemHandle,IN OUT HANDLE &hBin,IN int PAGE_SIZE,IN OUT MD5_CTX &md5Data)
{

	LARGE_INTEGER memOffset;

	PVOID mMap=NULL;
	DWORD failedMap=0;
	ULONGLONG successMap=0;
	DWORD bytesWritten=0;
	BOOL writeSuccess=FALSE;

	//Map a view of the portion of memory for this process to read from.

	memOffset.QuadPart=offset;
	SetLastError(0);
	mMap=MapViewOfFile(
		physmemHandle,
		FILE_MAP_READ,
		memOffset.HighPart,
		memOffset.LowPart,
		chunk
		);

	/*
	//Check if we have enough space on the device to proceed with capture
	if(!isSpaceEnough(&hBin,(DWORDLONG)GB3))
	{
		CleanUpOnExitFromDumpMemory64(hBin);
		return ERR_NOTENOUGHSPACE_x64;
	}
	*/
	
	if(!mMap)
	{
		AppLog.Write("Protected Memory Area",WSLMSG_STATUS);
		//Write a NULL page to the bin file
		//TCHAR g_tempBuf[MAX_PATH];
		//_sntprintf_s(g_tempBuf,MAX_PATH,MAX_PATH,L"Could not obtain view at offset %.8x %.8x",memOffset.HighPart,memOffset.LowPart);		
		//AppLog.Write(g_tempBuf,WSLMSG_NOTE);
		failedMap++;

		writeSuccess = WriteFile(hBin,nullPage,chunk,&bytesWritten,0);

		//Update md5 hash with NULL page hash
		MD5Update(&md5Data,nullPage,chunk);	

	}
	else
	{
		CopyMemory(chunkBuf,mMap,chunk);

		//Finally: Proceed as usual in filling up this file created above with pages captured
		writeSuccess = WriteFile(hBin,chunkBuf,chunk,&bytesWritten,0);


		//Update md5 hash
		MD5Update(&md5Data,chunkBuf,chunk);	
		//AppLog.Write("After MD5 update",WSLMSG_STATUS);

		//Now unmap the section
		UnmapViewOfFile(mMap);
	}

	if(!writeSuccess)
	{
		//Remove after use
		char msgBuf[MAX_PATH];
		sprintf(msgBuf,"Error in GrabChunkFromPhysicalMemory %d",GetLastError());
		AppLog.Write(msgBuf,WSLMSG_ERROR);
		//Remove after use
		AppLog.Write("Writing view to physmem.bin failed",WSLMSG_ERROR);
		return ERR_WRITEVIEWFAILED_x64;
	}

	// TODO: what is the proper return value for success?
	return 0;
}
	


/*--->
	EntryPoint in this file
<---*/

//Called from CapturePhysicalMemory in WSTRemote.cpp

/* 
02/18/2011 RJM -- Make changes to the physical memory capture of >4Gb. The physmem.bin will now 
be a set of files phymem1.bin, physmem2.bin etc. Each of these files would be 3Gb in size. In addition
to changes here the hashes of the memory files that get put into digest.xml will also change.


We need a null page to write to the memory dump in case a view to the physical memory is
not allowed. This could be because we might be reading from a physical memory area that 
is device memory (refer to http://blogs.technet.com/b/markrussinovich/archive/2008/07/21/3092070.aspx)
In these cases use the NULL_PAGE.*/

//These were all defined inside the DumpMemory64 function. Since the new gui the Abort would just kill the 
//process without unloading the driver. To avoid this we need to make these variables visible in WSTMain.cpp.
//call the cleanup function when abort is pressed.

HANDLE hBin = NULL;
SC_HANDLE hSCM=NULL;
SC_HANDLE hSVC=NULL;
HANDLE hphysmemDevice=INVALID_HANDLE_VALUE;

int WSTRemote::DumpMemory64(int arch64)//32 bit arch change
{
	VAR_DECLARE
		HANDLE physmemHandle = NULL; 
		
		MEMORYSTATUSEX ms;
	VAR_DECLARE

	
	memset(gui.updateMsg,0,sizeof(TCHAR)*MSG_SIZE);

	if(arch64)
	{

		AppLog.Write("Starting physical memory collection setup for 64-bit system",WSLMSG_STATUS);
	}
	else
	{
		AppLog.Write("Starting physical memory collection setup for 32-bit system",WSLMSG_STATUS);
	}
	
	//Check to see if we have admin privileges to collect memory
	if(!IsUserAnAdmin())
	{
		gui.DisplayWarning(_TEXT("Need to have administrative privileges to capture memory"));
		AppLog.Write("Need to have administrative privileges to dump memory",WSLMSG_ERROR);
		return ERR_NO_ADMIN_PRIVILEGE_x64;
	}

	VAR_DECLARE
		
		SYSTEM_INFO si;
	VAR_DECLARE

	//Determine PAGE_SIZE
	INC_SYS_CALL_COUNT(GetSystemInfo); // Needed to be INC TKH
	GetSystemInfo(&si);
	const DWORD PAGE_SIZE = si.dwPageSize;
	

	//Load the driver first by starting the sevice
	if (LoadDriver(hSCM,hSVC,hphysmemDevice,arch64) != DRIVER_LOAD_SUCCESS)//32 bit arch change
	{	
		AppLog.Write("Error loading the driver",WSLMSG_ERROR);
		return ERR_LOADING_DRIVER_x64;
	}

	//Try to open the handle to physical memory device  -- First call to the driver to obtain the handle to physmem
	
	if(OpenHandleToPhysicalMemory(&physmemHandle,hSCM,hSVC,hphysmemDevice) != FILE_OPEN_SUCCESS_x64)
	{
		AppLog.Write("Could not obtain handle to the physical memory device",WSLMSG_ERROR);
		return ERR_NOHANDLE_x64;
	}
	
	//Try and get the memory map -- Second call to the driver to obtain the firmware memory map
	
	VAR_DECLARE
		PE820_MAP map;
		int memory_count;	
		ULONG chunk_size = PAGE_SIZE*SPEEDUP;
		BYTE *nullPage=0;
		//This page holds the view data to be written to physmem.bin file
		BYTE *chunkBuf = 0;
	VAR_DECLARE
	
	chunkBuf = new BYTE[chunk_size];
	nullPage = new BYTE[chunk_size];
	memset(nullPage,0,chunk_size);
	
	if(GetMap(hphysmemDevice,&map,hSCM,hSVC) != ERROR_SUCCESS)
	{
		cout<<"Error returning from GetMap"<<endl;
		MemoryCaptureCleanUp(hSCM,hSVC,hphysmemDevice);
		return ERR_MAP_x64;
	}
	
	_sntprintf_s(msgBuf,MAX_PATH,MAX_PATH,L"Page Size %d",PAGE_SIZE);
	AppLog.Write(msgBuf,WSLMSG_STATUS);
		
	memset(msgBuf,0,MAX_PATH);

	//Setup hashing variables
	MD5_CTX md5Data;
	

	VAR_DECLARE
		int memory_region=0;
		ULONGLONG partial=0;
		ULONG total_number_of_chunks=0;
		os_memory_range *physical_memory_accesible;
		TCHAR fileName[MAX_PATH];
		ULONGLONG offset=0;
		ULONGLONG total_physical_memory_captured=0;
	VAR_DECLARE

	//Obtain the OS memory ranges
	gui.DisplayStatus(_TEXT("Obtaining OS Memory regions ..."));
	memory_count=GetOsMemoryRanges (map,&physical_memory_accesible);
	

	//Create the chunk_count_array for each of the memory regions identified.
	chunk_count_struct *chunk_count_array = new chunk_count_struct[memory_count];

	//Obtain total number of chunks to determine the percentage completion and couple that computation for the memory acquisition loop below.
	for(memory_region =0;memory_region < memory_count;memory_region++)
	{
		chunk_count_array[memory_region].chunk_count = compute_chunks(PAGE_SIZE,(physical_memory_accesible)[memory_region].start,(physical_memory_accesible)[memory_region].end,partial);
		chunk_count_array[memory_region].partial = partial;
		chunk_count_array[memory_region].start.QuadPart = (physical_memory_accesible)[memory_region].start.QuadPart;
		chunk_count_array[memory_region].end.QuadPart = (physical_memory_accesible)[memory_region].end.QuadPart;
		chunk_count_array[memory_region].size= (physical_memory_accesible)[memory_region].size;
		chunk_count_array[memory_region].type= (physical_memory_accesible)[memory_region].type;

		if(chunk_count_array[memory_region].type == 1)
		{
			total_physical_memory_captured += chunk_count_array[memory_region].size.QuadPart;
		}

		memset(msgBuf,0,MAX_PATH);
	
		_sntprintf_s(msgBuf,MAX_PATH,MAX_PATH,L"Memory Region %d has %d chunks and %lld bytes starts at 0x%08X`%08X and ends at 0x%08X`%08X in physical memory",
				memory_region,
				chunk_count_array[memory_region].chunk_count,
				chunk_count_array[memory_region].partial,
				chunk_count_array[memory_region].start.HighPart,
				chunk_count_array[memory_region].start.LowPart,
				chunk_count_array[memory_region].end.HighPart,
				chunk_count_array[memory_region].end.LowPart);
				

		AppLog.Write(msgBuf,WSLMSG_STATUS);


		total_number_of_chunks += chunk_count_array[memory_region].chunk_count;
		partial=0;//Reset the partial chunk value
	}

	ReleaseMap (map);//We can free the map resource now


	ms.dwLength=sizeof(MEMORYSTATUSEX);
	INC_SYS_CALL_COUNT(GlobalMemoryStatusEx); // Needed to be INC TKH
	GlobalMemoryStatusEx(&ms);

	memset(msgBuf,0,MAX_PATH);
	_sntprintf_s(msgBuf,MAX_PATH,MAX_PATH,L"Total physical memory available %lld bytes",total_physical_memory_captured);
	AppLog.Write(msgBuf,WSLMSG_STATUS);
	if(!isSpaceEnough(&hBin,total_physical_memory_captured))
	{
		MemoryCaptureCleanUp(hSCM,hSVC,hphysmemDevice);
		return ERR_NOTENOUGHSPACE_x64;
	}
	gui.DisplayStatus(_T("Starting memory capture ..."));
	gui.DisplayState(_TEXT("Capturing Memory ..."));
	//Initialize percentage counter
	percentComplete=0;
	int updateCounter=0;
	gui.SetPercentStatusBar();//Initialize the status bar;
	

	_stprintf(gui.updateMsg,_T("Capturing Physical Memory %d%% ..."),percentComplete);
	gui.DisplayResult(gui.updateMsg);

	memset(msgBuf,0,MAX_PATH);
	_sntprintf_s(msgBuf,MAX_PATH,MAX_PATH,L"Total number of chunks to capture %d",total_number_of_chunks);
	AppLog.Write(msgBuf,WSLMSG_STATUS);
	ULONG chunks_processed=0;
		
	LARGE_INTEGER fileSize;
	memset(&fileSize,0,sizeof(LARGE_INTEGER));

	
	//Map for MemoryRegion -> fileName.bin
	memory_region_file_map = new memory_map[memory_count];
	int file_map_count=0;
	//Map for MemoryRegion -> fileName.bin
#ifdef _DEBUG
	cout<<"Physical Memory capture code around GrabChunkFromPhysicalMemory disabled in DEBUG mode (Uncomment to capture memory)"<<endl;
#endif
	for(memory_region=0;memory_region < memory_count;memory_region++)
	{
			
			int chunk_count=0;
	
			

			if(chunk_count_array[memory_region].type == 1)
			{
				memset(&md5Data,0,sizeof(MD5_CTX));//Reinitalize hashing variables
				MD5Init(&md5Data);
				hBin=CreatePhysmemFile(fileName); // Open a new file for each memory region 
			}

			//Add entry to memory->file map
			memory_region_file_map[file_map_count].start=chunk_count_array[memory_region].start;
			memory_region_file_map[file_map_count].end=chunk_count_array[memory_region].end;
			memory_region_file_map[file_map_count].size=chunk_count_array[memory_region].size;
			memory_region_file_map[file_map_count].type=chunk_count_array[memory_region].type;
			//Specific to usable RAM
			if(chunk_count_array[memory_region].type == 1)
			{
				
				wmemset(memory_region_file_map[file_map_count].fileName[0],0x0,30);
				wcout<<"Length of "<<fileName<<" "<<_tcslen(fileName)<<endl;
				_tcsncpy(memory_region_file_map[file_map_count].fileName[0],fileName,_tcslen(fileName));
				wmemset(memory_region_file_map[file_map_count].fileName[1],0x0,30);
				wmemset(memory_region_file_map[file_map_count].fileName[2],0x0,30);
			}
			else
			{
				wmemset(memory_region_file_map[file_map_count].fileName[0],0x0,30);
				wmemset(memory_region_file_map[file_map_count].fileName[1],0x0,30);
				wmemset(memory_region_file_map[file_map_count].fileName[2],0x0,30);
			}
			memory_region_file_map[file_map_count].final=false;

			//Add entry to memory->file map
			

			if(chunk_count_array[memory_region].type != 1)
			{
				file_map_count+=1;
				continue;//Not an accessible memory region
			}

			if(hBin == INVALID_HANDLE_VALUE)
			{
				AppLog.Write("Could not create physmem.bin file x64",WSLMSG_ERROR);
				//CleanUp and return TODO
				return ERR_PHYSMEMFILE_x64;
			}

			
			//Proceed with memory capture

			offset =chunk_count_array[memory_region].start.QuadPart; //Start of the region

			
			int region_file_counter;
			region_file_counter=1;

			for(chunk_count=0;chunk_count < chunk_count_array[memory_region].chunk_count;chunk_count ++)
			{
				updateCounter=percentComplete;
				//Abort logic
				if((currentState == ABORTING || currentState == ABORTED) && (region_file_counter < 3 ))
				{

					AppLog.Write("Cleaning Up memory used for physical memory acquisition",WSLMSG_STATUS);
					CleanUpOnExitFromDumpMemory64(hBin,hSCM,hSVC,hphysmemDevice);
					AppLog.Write("Aborting physical memory dump x64",WSLMSG_STATUS);
					if(region_file_counter >= 3)
						AppLog.Write("Aborting phsyical memory dump x64: Cannot fragment the region into anymore 4Gb chunks (allocate more)",WSLMSG_STATUS);
					return MEMORY_DUMP_SUCCESSFUL;
				}
				//Abort logic

				//FAT cannot create a file bigger than 4GB
				GetFileSizeEx(hBin,&fileSize);
				if(fileSize.QuadPart > FOURGBLIMIT)
				{
					AppLog.Write("File size limit reached, creating new file",WSLMSG_STATUS);
					if(hBin)
					{
						INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
						CloseHandle(hBin);
					}//Close the handle to the bin file 

					writeHashToDigest(const_cast<TCHAR *>(fileName),md5Data,m_hasher);//Write hash to digest file

					memset(&md5Data,0,sizeof(MD5_CTX));
					MD5Init(&md5Data);
					hBin=CreatePhysmemFile(fileName); // Open a new file for each memory region 
					_tcsncpy(memory_region_file_map[file_map_count].fileName[region_file_counter],fileName,_tcslen(fileName));//Add to the memory->file map (dont envisage more, could be wrong in 20 years from now).

					if(hBin == INVALID_HANDLE_VALUE)
					{
						AppLog.Write("Could not create physmem.bin file x64",WSLMSG_ERROR);
						//CleanUp and return TODO
						return ERR_PHYSMEMFILE_x64;
					}
					memset(&fileSize,0,sizeof(LARGE_INTEGER));
					region_file_counter+=1;
				}

				memset(chunkBuf,0,chunk_size);
				
#ifndef _DEBUG
				
				if(GrabChunkFromPhysicalMemory(offset,chunk_size,chunkBuf,nullPage,physmemHandle,hBin,PAGE_SIZE,md5Data) == ERR_WRITEVIEWFAILED_x64)
				{
					CleanUpOnExitFromDumpMemory64(hBin,hSCM,hSVC,hphysmemDevice);
					FreeResources(chunkBuf,nullPage,chunk_count_array);
					return ERR_WRITEVIEWFAILED_x64;
				}
#endif
				
				offset += chunk_size;
				percentComplete = (int)( (((double)chunks_processed)/(total_number_of_chunks)) * 100);//Update percentage completed that will be sent to fontend through pipehandler
				//Send percentage updates to the dialog box
				
				//if(percentComplete > updateCounter)
				{
					memset(gui.updateMsg,0,sizeof(TCHAR)*MSG_SIZE);

					_stprintf(gui.updateMsg,_T("Capturing Physical Memory %d%% ..."),percentComplete);
					gui.DisplayResult(gui.updateMsg);

					SendMessage(gui.hProgress, PBM_STEPIT, 0, 0); 
				}
				//Send percentage updates to the dialog box
				chunks_processed++;

			}
#ifndef _DEBUG
			if(chunk_count_array[memory_region].partial != 0)
			{
				memset(chunkBuf,0,chunk_size);
				if(GrabChunkFromPhysicalMemory(offset,chunk_count_array[memory_region].partial,chunkBuf,nullPage,physmemHandle,hBin,PAGE_SIZE,md5Data) == ERR_WRITEVIEWFAILED_x64)
				{
					CleanUpOnExitFromDumpMemory64(hBin,hSCM,hSVC,hphysmemDevice);
					FreeResources(chunkBuf,nullPage,chunk_count_array);
					return ERR_WRITEVIEWFAILED_x64;
				}
			}
#endif
			
			memset(msgBuf,0,MAX_PATH);
			_sntprintf_s(msgBuf,MAX_PATH,MAX_PATH,L"Finished capturing memory from region %d",memory_region);
			AppLog.Write(msgBuf,WSLMSG_STATUS);

			if(hBin)
			{
				INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
				CloseHandle(hBin);
			}//Close the handle to the bin file 

			writeHashToDigest(const_cast<TCHAR *>(fileName),md5Data,m_hasher);//Write hash to digest file
			file_map_count += 1; //next memory -> map 
	}
	memory_region_file_map[memory_count-1].final=true;
	//Done
	percentComplete=100;
	memset(gui.updateMsg,0,sizeof(TCHAR)*MSG_SIZE);

	_stprintf(gui.updateMsg,_T("Capturing Physical Memory %d%% ..."),percentComplete);
	gui.DisplayResult(gui.updateMsg);
	SendMessage(gui.hProgress, PBM_SETPOS, 100, 0); //Complete
	AppLog.Write("Physical memory dump complete",WSLMSG_STATUS);
	
	AppLog.Write("Cleaning Up memory used for physical memory acquisition",WSLMSG_STATUS);

	MemoryCaptureCleanUp(hSCM,hSVC,hphysmemDevice);
	FreeResources(chunkBuf,nullPage,chunk_count_array);
	AppLog.Write("Exiting phsyical memory dump x64",WSLMSG_STATUS);


	return MEMORY_DUMP_SUCCESSFUL;
}

#undef UNICODE
#undef _UNICODE

