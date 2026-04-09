#define UNICODE
#define _UNICODE
#ifdef MAILFILECAPTURE
#include "MailCapture.h"
#pragma comment(linker, "/defaultlib:EmailExtract.lib")



#ifdef PRO
	extern TCHAR caseDrive[];
	extern TCHAR uslattDrive[];
	int MailFileCapture(WSTLog& AppLog, gui_resources& gui, TCHAR caseDrive[], TCHAR uslattDrive[]) { return 0; }
#else
	int MailFileCapture(WSTLog &AppLog, gui_resources &gui);
#endif

DWORD errorCode;

DWORD WINAPI MailThread(LPVOID eParams)
{
	TCHAR msgBuf[MAX_PATH];
	memset(msgBuf,0,MAX_PATH);
	//recast it to be not a void pointer
	Args* params = (Args*) (eParams);
	gui_resources* guiRef = &(params->GUI);
	WSTLog* al = &(params->AL);
	//pass in the applog and gui
#ifdef PRO
	errorCode = MailFileCapture(*al, *guiRef,caseDrive,uslattDrive);
#else
	errorCode = MailFileCapture(*al, *guiRef);
#endif

	_stprintf_s(msgBuf, MAX_PATH, TEXT("Mail Files Capture thread returned %d"), errorCode);

	(*al).Write(msgBuf,WSLMSG_STATUS);

	return errorCode;
}

int WSTRemote::CaptureMail()
{
	gui.SetPercentStatusBar(10,0,100);
	gui.ResetUpdateCounter();

	//Thread Related 
	DWORD threadId = 0;
	DWORD thread_return = 0;
	HANDLE himagingThread = INVALID_HANDLE_VALUE;
	// Created a struct for my params, see struct dec in MailCapture.h
	Args eParams ={AppLog, gui};
	//you can't send multiple params w/createthread, thus make a struct.
	himagingThread = CreateThread(NULL,0, MailThread,(void *) &eParams,0,&threadId);
	if(himagingThread == NULL)
	{
		AppLog.Write("Mail Files Capture thread failed during creation",WSLMSG_ERROR);
		currentState = MAIL_COMPLETE;
		return ERR_MAIL_CAPTURE_THREAD_CREATE_ERROR;
	}

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
			if(errorCode == ERROR_SUCCESS)
			{
				currentState = MAIL_COMPLETE;
			}
			else
			{
				AppLog.Write("Mail Capture Failed",WSLMSG_ERROR);
			}
			break;
		}
		else if(thread_return == WAIT_TIMEOUT)
		{
			continue;
		}
	}
	if(errorCode == CPY_INSUFF_SPACE_ERR)
	{
		gui.DisplayWarning(_T("Mail capture failed due to insufficient space ..."));
		AppLog.Write("Mail Capture Unsuccessful due to insufficient space on the token",WSLMSG_ERROR);
	}
	gui.ResetProgressBar();
}
#endif
#undef UNICODE
#undef _UNICODE