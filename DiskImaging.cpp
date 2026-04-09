

#ifdef DRIVE_IMAGING //Define this preprocessor in the settings to enable this feature


//NOTE: The error codes are the same as that for TrueCrypt drive imaging.
#define _UNICODE 
#define UNICODE



#pragma comment(linker, "/defaultlib:wstdd.lib")

#include "WSTRemote.H"
#include "WSTLog.h"
#include "WSTDD.h"
#include "hashedxml.h"
#include <map>
#include <ShlObj.h>

#ifdef PRO
	extern TCHAR caseDrive[];
	extern TCHAR uslattDrive[];
	int wstdd(IN const TCHAR *inputDrive,IN const TCHAR *ddImageName,OUT char *hashString,IN TCHAR caseDrive[],IN TCHAR uslattDrive[],gui_resources &gui);
#else
	extern int wstdd(IN const char *inputDrive,IN const char *ddImageName,OUT char *hashString);
#endif
extern int WriteHashes(tstring filename=_T("NULL"),tstring hashValue=_T("NULL"));

ImagingStruct *imageParams;



extern DWORD WINAPI ImagingThread(__in  LPVOID iparams);
void GetNextImageFileName(char *imageFileName,char imagePath[100]);
TCHAR * ConvertToAppropriateBytePrefix(TCHAR *message,ULARGE_INTEGER &totalNumberOfBytes,long long physicalMemory=0);

int WSTRemote::ImageSelectedExternalDrives(HashedXMLFile &repository)
{
	

	AppLog.Write("Starting Drive Imaging of selected drives",WSLMSG_STATUS);

	//General
	char driveLetter; 
	char imageFileName[MAX_PATH+1];
	char hashString[40];
	wchar_t imageFileNameW[MAX_PATH * 2+1];
	wchar_t hashStringW[80];
	wstring volumePath;
	TCHAR sizeString[20];
	int retCode=IMAGING_SUCCESSFUL;
	
	currentState = IMAGING_START;//For frontend
	
	gui.DisplayStatus(_T("Checking for drives to Image"));
	//Thread related
	ImagingStruct imageParams={NULL,NULL,NULL,gui,0};
	DWORD threadId=0;
	DWORD thread_return=0;
	HANDLE himagingThread=INVALID_HANDLE_VALUE;

	if(!IsUserAnAdmin())
	{
		gui.DisplayResult(_T("Imaging Complete"));
		gui.DisplayWarning(_T("Need to have administrative privileges to capture TrueCrypt Image"));
		AppLog.Write("Drive Imaging Failed -- Needs administrative privileges",WSLMSG_ERROR);
		currentState = IMAGING_COMPLETE;
		return ERR_NOT_AN_ADMIN;
	}

	repository.OpenNode("DiskImages");

	//1. Iterate ove all the drives selected to image.
	for(map<TCHAR,driveImaging*>::iterator dit=gui.imageDriveMap.begin();dit!= gui.imageDriveMap.end();++dit)
	{
		//Reset all the locals
		memset(sizeString,0,sizeof(TCHAR)*20);
		memset(imageFileName,0,MAX_PATH+1);
		memset(driveToImage,0,3);
		memset(hashString,0,40);
		memset(imageFileNameW,0,MAX_PATH*2+1);
		memset(hashStringW,0,80);
		

		gui.DisplayState(_T("Imaging Drives"));

		//2. For each drive check to see if the imagingSelected property is true. If true setup data sturctures for imaging
		if(dit->second->imagingSelected)
		{
			//Set the gui result value
			gui.SetPercentStatusBar(10,0,100);
			gui.ResetUpdateCounter();
			memset(gui.updateMsg,0,sizeof(TCHAR)*MSG_SIZE);
			_stprintf_s(gui.updateMsg,40,_T("Imaging Drive %s"),dit->second->driveName);
			gui.DisplayState(gui.updateMsg);
			gui.DisplayStatus(gui.updateMsg);
			gui.DisplayResult(_T("Imaging In Progress ..."));


			//2a. Create and entry in the XML file listing all the properties of the drive to be imaged along-with the name of the image file.

			//Get the next suitable name for the image
			GetNextImageFileName(imageFileName,RAWIMAGENAME);

			//Write attributes of the drive
			repository.OpenNode("Image");
			repository.WriteNodeWithValue("Drive",dit->second->driveName);
			repository.WriteNodeWithValue("Type",dit->second->driveType);
			repository.WriteNodeWithValue("FileSystem",dit->second->fileSystem);
			repository.WriteNodeWithValue("VolumeName",dit->second->volumeName);
			repository.WriteNodeWithValue("Size",ConvertToAppropriateBytePrefix(_T(""),(dit->second->size),0));
			repository.WriteNodeWithValue("ImageLocation",imageFileName);
			

			//wstdd likes it in the format C:
			driveToImage[0]=dit->first;
			driveToImage[1]=':';
		
			//2b. Setup an imageParams structure (should be re-used for other drives);

			//Assignments for the imaging thread in the ImagingStruct
			imageParams.ddImageName = imageFileName;
			imageParams.hashString  = hashString;
			imageParams.inputDrive  = driveToImage;
		

			//3.Time to create a thread and call the WSTDD module for imaging.

			himagingThread = CreateThread(NULL,0,ImagingThread,(void *)&imageParams,0,&threadId);

			if(himagingThread == NULL)
			{
				AppLog.Write("Drive Imaging thread failed during creation",WSLMSG_ERROR);
				repository.WriteNodeWithValue("Result",_T("FAILED"));
				repository.CloseNode("Image");
				continue;//Move to the next one
			}

			//Wait for thread to terminate or return when triage is aborted

			while(1)
			{
				if(gui.TIMETOUPDATE())
				{
					Sleep(2000);
					SendMessage(gui.hProgress,PBM_STEPIT,0,0);
				}
				else
				{
					gui.IncrementUpdateCounter();
					gui.CheckOverFlow();
				}
				thread_return = WaitForSingleObject(himagingThread,0);


				if(thread_return == WAIT_OBJECT_0)
				{
					if(imageParams.error_code == IMAGING_SUCCESSFUL)
					{
						
						gui.DisplayResult(_T("Imaging Complete"));
						gui.DisplayStatus(_T("Imaging Successful"));
					} 
					else
					{
						gui.DisplayResult(_T("Imaging Complete"));
						gui.DisplayError(_T("Drive Imaging Failed"));
						retCode=imageParams.error_code;
						AppLog.Write("Drive Imaging Failed",WSLMSG_ERROR);
					}
					break;
				}
				else if(thread_return == WAIT_TIMEOUT)
				{
					continue;
				}
			}

			if(imageParams.error_code == ERR_WRITE_FILE_FAILED)
			{
				
				gui.DisplayResult(_T("Imaging Complete"));
				gui.DisplayError(_T("Drive Imaging Failed: Write to device failed (possibly due to use of FAT/FAT32 filesystem)"));
				AppLog.Write("Drive imaging unsuccessful due to write failure to the storage device",WSLMSG_ERROR);
				repository.WriteNodeWithValue("Result",_T("WRITE FAILED"));

				mbstowcs(hashStringW,hashString,80);
				mbstowcs(imageFileNameW,imageFileName,2*MAX_PATH+1);
				wstring formattedFileNameW = L"\\"+ wstring(imageFileNameW); //needless waste of space but necessary
				WriteHashes(formattedFileNameW,hashStringW);

				AppLog.Write(formattedFileNameW.c_str(),WSLMSG_ERROR);
				AppLog.Write(hashStringW,WSLMSG_ERROR);

				retCode=ERR_WRITE_FILE_FAILED;
			}

			if(imageParams.error_code == ERR_NOT_ENOUGH_SPACE_AVAILABLE)
			{
				gui.DisplayResult(_T("Imaging Complete"));
				gui.DisplayWarning(_T("Drive Imaging Failed: Insufficient space on the storage device"));
				AppLog.Write("Drive imaging unsuccessful due to insufficient space on the storage device",WSLMSG_ERROR);
				repository.WriteNodeWithValue("Result",_T("INSUFFICIENT SPACE"));
				repository.CloseNode("Image");
				currentState = DISK_FULL;//Indicates that the imaging was incomplete due to lack of space on the device.		
				retCode=ERR_NOT_ENOUGH_SPACE_AVAILABLE;
				break;//Break out of the for loop since there is no space left on the device
			}

			//4. Write hash to digest.xml
			if(imageParams.error_code == IMAGING_SUCCESSFUL)
			{
				repository.WriteNodeWithValue("Result",_T("SUCCESS"));
				mbstowcs(hashStringW,hashString,80);
				mbstowcs(imageFileNameW,imageFileName,2*MAX_PATH+1);
				wstring formattedFileNameW = L"\\"+ wstring(imageFileNameW); //needless waste of space but necessary
				WriteHashes(formattedFileNameW,hashStringW);
			}
			repository.CloseNode("Image");
		}//Closes if(dit->second->imagingSelected)
	}//Closes the for loop

	repository.CloseNode("DiskImages");

	gui.ResetProgressBar();
	AppLog.Write("Drive Imaging Completed",WSLMSG_STATUS);
	return retCode;
		
}


#undef _UNICODE
#undef UNICODE


#endif