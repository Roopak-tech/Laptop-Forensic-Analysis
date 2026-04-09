

#ifdef TRUECRYPT //Define this preprocessor in the settings to enable this feature


#define _UNICODE 
#define UNICODE



#pragma comment(linker, "/defaultlib:wstdd.lib")

#include "WSTRemote.H"
#include "WSTLog.h"
#include "WSTDD.h"
#include "hashedxml.h"
#include <ShlObj.h>

#ifdef PRO
	extern TCHAR caseDrive[];
	extern TCHAR uslattDrive[];
	int wstdd(IN const TCHAR *inputDrive,IN const TCHAR *ddImageName,OUT char *hashString,IN TCHAR caseDrive[],IN TCHAR uslattDrive[],gui_resources &gui);
#else
	extern int wstdd(IN const char *inputDrive,IN const char *ddImageName,OUT char *hashString);
#endif
extern int WriteHashes(tstring filename=_T("NULL"),tstring hashValue=_T("NULL"));

int RetFromTCImaging;//Shabby


//Return a new binary file name for capture
void GetNextImageFileName(char *imageFileName,char imagePath[100])
{

	static int volume_count = 0;

	int len = MAX_PATH;
	char cCount[10];
	memset(cCount,0,10);

	strncpy(imageFileName,imagePath,MAX_PATH);
	len=len - strlen(imageFileName);
	itoa(++volume_count,cCount,10);
	strncat(imageFileName,cCount,len);
	strncat(imageFileName,".bin",4);
}


ImagingStruct imagingParams={NULL,NULL,NULL,gui,0};



DWORD WINAPI ImagingThread(__in  LPVOID iparams)
{
	const char *driveToImage;
	const char *imageFileName;
	char *hashString;
	
	

	TCHAR msgBuf[MAX_PATH];
	memset(msgBuf,0,MAX_PATH);

	ImagingStruct *params = (ImagingStruct *)(iparams); 

	imageFileName = params->ddImageName;
	hashString = params->hashString;
	driveToImage = params->inputDrive;
	gui_resources &gui=params->gui;

#ifdef PRO
	TCHAR driveToImageW[20];
	TCHAR imageFileNameW[MAX_PATH+1];
	memset(driveToImageW,0,20*sizeof(TCHAR));
	memset(imageFileNameW,0,(MAX_PATH+1)*sizeof(TCHAR));
	mbstowcs(driveToImageW,driveToImage,20);
	mbstowcs(imageFileNameW,imageFileName,MAX_PATH);
	params->error_code = wstdd(driveToImageW,imageFileNameW,hashString,caseDrive,uslattDrive,gui);
#else
	params->error_code = wstdd(driveToImage,imageFileName,hashString,caseDrive,uslattDrive);
#endif

	swprintf_s(msgBuf,MAX_PATH,L"TrueCrypt imaging thread returned %d",params->error_code);

	AppLog.Write(msgBuf,WSLMSG_STATUS);

	return params->error_code;
}

//Image TrueCrypt Drives

int WSTRemote::ImageTrueCryptDrives(HashedXMLFile &repository)
{
	gui.SetPercentStatusBar(10,0,100);
	gui.ResetUpdateCounter();

	AppLog.Write("Starting TrueCrypt volume detection/imaging",WSLMSG_STATUS);

	//General
	char driveLetter; 
	char imageFileName[MAX_PATH+1];
	char hashString[40];
	wchar_t imageFileNameW[MAX_PATH * 2+1];
	wchar_t hashStringW[80];
	wstring volumePath;

	//Thread related
	DWORD threadId=0;
	DWORD thread_return=0;
	HANDLE himagingThread=INVALID_HANDLE_VALUE;


	repository.OpenNode("EncryptedFileSystemImages");


	//First identify all the truecrypt drives
	gui.DisplayStatus(_T("Identifying TrueCrypt Drives..."));
	for(driveLetter = 'A';driveLetter <= 'Z';driveLetter++ )
	{
		gui.DisplayResult(_T("Identifying ..."));

		//Check if the drive is a truecrypt drive
		if(IsTrueCryptDrive(driveLetter,volumePath))
		{
			//Reset and zero all locals
			memset(imageFileName,0,MAX_PATH+1);
			memset(driveToImage,0,3);
			memset(hashString,0,40);
			memset(imageFileNameW,0,MAX_PATH*2+1);
			memset(hashStringW,0,80);

			//wstdd likes it as S:
			driveToImage[0]=driveLetter;
			driveToImage[1]=':';

			//Get the next suitable name for the image
			GetNextImageFileName(imageFileName,IMAGENAME);

			//Let's start
			currentState = IMAGING_START;//For frontend

			//Assignments for the imaging thread in the ImagingStruct
			imagingParams.ddImageName = imageFileName;
			imagingParams.hashString  = hashString;
			imagingParams.inputDrive  = driveToImage;
			
		

			//Call the thread
			memset(gui.updateMsg,0,sizeof(TCHAR)*MSG_SIZE);
			TCHAR wtc[10];
			memset(wtc,0,sizeof(TCHAR)*10);
			mbstowcs(wtc,driveToImage,10);
			_stprintf(gui.updateMsg,_TEXT("Found TrueCrypt drive %s"),wtc);
			gui.DisplayStatus(gui.updateMsg);

			gui.DisplayStatus(_T("Imaging TrueCrypt Drive ..."));
			gui.DisplayResult(_T("Imaging In Progress ..."));
			if(IsUserAnAdmin())
			{

				himagingThread = CreateThread(NULL,0,ImagingThread,(void *)&imagingParams,0,&threadId);

				if(himagingThread == NULL)
				{
					AppLog.Write("TrueCrypt imaging thread failed during creation",WSLMSG_ERROR);
					currentState = IMAGING_COMPLETE;
					return ERR_TRUECRYPT_THREAD_CREATE_ERROR;
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
						if(imagingParams.error_code == IMAGING_SUCCESSFUL)
						{
							gui.DisplayResult(_T("Imaging Complete"));
							gui.DisplayStatus(_T("TrueCrypt Imaging Successful"));
							//Write the bin file information to results.xml
							repository.OpenNode("Image");
							repository.WriteNodeWithValue("Drive",driveToImage);
							repository.WriteNodeWithValue("Type","TrueCrypt");
							repository.WriteNodeWithValue("ImageLocation",imageFileName);
							repository.CloseNode("Image");
						} 
						else
						{
							gui.DisplayResult(_T("Imaging Complete"));
							gui.DisplayError(_T("TrueCrypt Imaging Failed"));
							AppLog.Write("TrueCrypt Imaging Failed",WSLMSG_ERROR);
						}
						break;
					}
					else if(thread_return == WAIT_TIMEOUT)
					{
						continue;
					}
				}

				if(imagingParams.error_code == ERR_NOT_ENOUGH_SPACE_AVAILABLE)
				{
					gui.DisplayResult(_T("Imaging Complete"));
					gui.DisplayError(_T("TrueCrypt Imaging Failed: Insufficient space on the token"));
					AppLog.Write("TrueCrypt Imaging Unsuccessful due to insufficient space on the token",WSLMSG_ERROR);
					currentState = DISK_FULL;//Indicates that the imaging was incomplete due to lack of space on the device.		
				}
				else if(imagingParams.error_code == ERR_WRITE_FILE_FAILED)
				{

					gui.DisplayResult(_T("Imaging Complete"));
					gui.DisplayError(_T("TrueCrypt Imaging Failed: Write to device failed (possibly due to use of FAT/FAT32 filesystem)"));
					AppLog.Write("TrueCrypt Imaging was unsuccessful due to write failure to the storage device",WSLMSG_ERROR);
					currentState = IMAGING_FAILED;
				}
				else
				{
					currentState = IMAGING_COMPLETE;//For frontend to indicate we are done.
				}


				//Steps to write to digest.xml
				if(imagingParams.error_code == IMAGING_SUCCESSFUL)
				{
					mbstowcs(hashStringW,hashString,80);
					mbstowcs(imageFileNameW,imageFileName,2*MAX_PATH+1);
					wstring formattedFileNameW = L"\\"+ wstring(imageFileNameW); //needless waste of space but necessary
					WriteHashes(formattedFileNameW,hashStringW);
				}
#ifdef _DEBUG
				cout<<hashString<<endl;
#endif
			}
			else
			{
				gui.DisplayResult(_T("Imaging Complete"));
				gui.DisplayError(_T("Need to have administrative privileges to capture TrueCrypt Image"));
				AppLog.Write("TrueCrypt Imaging Failed -- Needs administrative privileges",WSLMSG_ERROR);
				currentState = IMAGING_COMPLETE;
			}
		}

	}

	repository.CloseNode("EncryptedFileSystemImages");


	if(currentState == IMAGING_COMPLETE)
	{
		AppLog.Write("TrueCrypt Imaging Finished",WSLMSG_STATUS);
		return IMAGING_SUCCESSFUL;
	}
	else
	{
		return imagingParams.error_code;
	}


}


#undef _UNICODE
#undef UNICODE


#endif
