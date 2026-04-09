
#pragma once 

#include <WINDOWS.H>
#include <CommCtrl.h> //Status bar controls
#include <iostream>
#include <vector>
#include <map>
using namespace std;

//Variables
#ifndef STRIKE
const TCHAR g_szClassName[] = TEXT("US-LATT");
#else
const TCHAR g_szClassName[] = TEXT("Strike Live");
#endif

//Errors
#define ERR_CRITICAL_VALIDATION -101
#define ERR_CRITICAL_SETUPCASEDIRECTORY -102
#define ERR_CRITICAL_UNKNOWN -103
#define ERR_MAXSCANS_EXCEEDED -104 
#define MSG_SIZE 40
#define LARGE_MSG_SIZE MAX_PATH+1+100
#define UPDATE_INTERVAL 10000
#define UPDATE_LIMIT 100000
#ifdef DRIVE_IMAGING
enum {GB1=1024*1024*1024,MB1=1024*1024,KB1=1024};


struct driveImaging
{
public:
	bool imagingSelected;
	TCHAR driveName[10];
	ULARGE_INTEGER size;
	TCHAR driveType[30];
	TCHAR volumeName[MAX_PATH+1];
	TCHAR fileSystem[10];

	driveImaging()
	{
		imagingSelected = false;
		memset(&size,0,sizeof(ULARGE_INTEGER));
		memset(driveName, 0,sizeof(TCHAR)*10);
		memset(driveType, 0,sizeof(TCHAR)*30);
		memset(volumeName,0,sizeof(TCHAR)*(MAX_PATH+1));
		memset(fileSystem,0,sizeof(TCHAR)*10);

	}
};
#endif
 class gui_resources
{
public:
~ gui_resources()
{

#ifdef DRIVE_IMAGING
	/*for(map<TCHAR,driveImaging *>::iterator it=imageDriveMap.begin();it!= imageDriveMap.end();it++)
	{
		delete [] it->second;
	}*/
	imageDriveMap.clear();
#endif

	cout << "~gui" << endl;
}

 gui_resources()
 {
#ifdef PRO
	msgCount=0;
#endif
	MessageCount=0;
	errorCount=0;
	warningCount=0;
	updateCounter=0;

	
	herrorButton=NULL;
	hejectButton=NULL;
	hstartButton=NULL;
	hwarningButton=NULL;
	habortButton=NULL;
	statusMsgLB=NULL;
	msgLB=NULL;
	hStatus=NULL;
	DialogHwnd=NULL;
#ifdef DRIVE_IMAGING
	hDriveImageButton=NULL;
	hDriveImageWindow=NULL;
	hDialogBoxForDriveSelection=NULL;
	isUserAdmin=true;
	isTriageInProgress=false;
#endif
	hProgress=NULL;
	hInst=NULL;
	uslatt_icon=NULL;
	detail=true;

	memset(&stateRect,0,sizeof(RECT));
	memset(updateMsg,0,sizeof(TCHAR)*MSG_SIZE);
	memset(largeUpdateMsg,0,sizeof(TCHAR)*LARGE_MSG_SIZE);
#ifdef DRIVE_IMAGING
	memset(currentDropDownDriveString,0,sizeof(TCHAR)*(MAX_PATH+1));
	memset(&totalBytesToImage,0,sizeof(ULARGE_INTEGER));
	memset(&totalFreeSpace,0,sizeof(LARGE_INTEGER));
#endif
 }
//variables
public:
	int MessageCount;
	int errorCount;
	int warningCount;
	int updateCounter;
	RECT stateRect;
	HWND herrorButton;
	HWND hejectButton;
	HWND hstartButton;
	HWND hwarningButton;
	HWND habortButton;
	HWND statusMsgLB;
	HWND msgLB;
	HWND hStatus;
	HWND DialogHwnd;
#ifdef DRIVE_IMAGING
	HWND hDriveImageWindow;
	HWND hDriveImageButton;
	HWND hDialogBoxForDriveSelection;
	TCHAR currentDropDownDriveString[MAX_PATH+1];
	ULARGE_INTEGER totalBytesToImage;
	ULARGE_INTEGER totalFreeSpace;
	map<TCHAR,driveImaging*> imageDriveMap;//"C:\ D:\ E:\" 
	boolean isUserAdmin;
	boolean isTriageInProgress;
#endif
	HWND hProgress;
	HINSTANCE hInst;
	
	TCHAR updateMsg[MSG_SIZE];
	TCHAR largeUpdateMsg[LARGE_MSG_SIZE];

	 HICON uslatt_icon;
	 bool detail;
#ifdef PRO
	int msgCount;
	
#endif

//functions
public:
	void DisplayStatus(TCHAR *msg)
	{
		SendMessage(statusMsgLB, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)(msg));
#ifdef PRO
		msgCount=SendMessage(statusMsgLB,LB_GETCOUNT,0,0); //Logic to perform autoscroll in the listbox as and when the messages appear.
		if(msgCount != LB_ERR && msgCount > 10)
		{
			SendMessage(statusMsgLB,LB_SETCURSEL,msgCount-1,0);
		}
#endif

	}
	
	void DisplayError(TCHAR *msg)
	{
		TCHAR *errMsg;
		errMsg=new TCHAR[_tcslen(msg)+1+MSG_SIZE];
		memset(errMsg,0,sizeof(TCHAR)*(_tcslen(msg)+1+MSG_SIZE));
		_stprintf(errMsg,_T("ERROR: %s"),msg);
		SendMessage(statusMsgLB, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)(errMsg));
#ifdef PRO
		msgCount=SendMessage(statusMsgLB,LB_GETCOUNT,0,0); //Logic to perform autoscroll in the listbox as and when the messages appear.
		if(msgCount != LB_ERR && msgCount > 10)
		{
			SendMessage(statusMsgLB,LB_SETCURSEL,msgCount-1,0);
		}
#endif
		errorCount +=1;
		delete []errMsg;
	}

	void DisplayWarning(TCHAR *msg)
	{
		TCHAR *warningMsg;
		warningMsg=new TCHAR[_tcslen(msg)+1+MSG_SIZE];
		memset(warningMsg,0,sizeof(TCHAR)*(_tcslen(msg)+1+MSG_SIZE));
		_stprintf(warningMsg,_T("WARNING: %s"),msg);
		SendMessage(statusMsgLB, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)(warningMsg));
#ifdef PRO
		msgCount=SendMessage(statusMsgLB,LB_GETCOUNT,0,0); //Logic to perform autoscroll in the listbox as and when the messages appear.
		if(msgCount != LB_ERR && msgCount > 10)
		{
			SendMessage(statusMsgLB,LB_SETCURSEL,msgCount-1,0);
		}
#endif
		warningCount +=1;
		delete []warningMsg;
	}
	void DisplayState(TCHAR *msg)
	{
		SendMessage(hStatus,SB_SETTEXT,0,(LPARAM)(LPCTSTR)(msg));
	}

	void DisplayResult(TCHAR *msg)
	{
		SendMessage(hStatus,SB_SETTEXT,1,(LPARAM)(LPCTSTR)(msg));
	}

	void ResetProgressBar()
	{
		SendMessage(hProgress, PBM_SETPOS, 0, 0); //Reset
	}

	void ResetUpdateCounter()
	{
		updateCounter=0;
	}
	void CheckOverFlow()
	{
		if(updateCounter > UPDATE_LIMIT)
		{
			updateCounter=0;
		}
	}
	void IncrementUpdateCounter()
	{
		updateCounter+=1;
	}
	int TIMETOUPDATE()
	{
		return (!(updateCounter % UPDATE_INTERVAL));
	}
	void SetPercentStatusBar(int step=1,int low=0,int high=100)
	{
		SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(low,high)); 
		SendMessage(hProgress, PBM_SETSTEP, MAKEWPARAM(step, 0), 0);
	}

	void EnableErrorButton()
	{
		TCHAR errorLabel[30];
		memset(errorLabel,0,sizeof(TCHAR)*30);
		_stprintf(errorLabel,_T("View Errors (%d)"),errorCount);
		SetWindowText(herrorButton,errorLabel);
		EnableWindow (herrorButton,TRUE);
	}
	void EnableWarningButton()
	{
		TCHAR warningLabel[30];
		memset(warningLabel,0,sizeof(TCHAR)*30);
		_stprintf(warningLabel,_T("View Warnings (%d)"),warningCount);
		SetWindowText(hwarningButton,warningLabel);
		EnableWindow (hwarningButton,TRUE);
	}

	void DisableWarningButton()
	{
		TCHAR warningLabel[30];
		memset(warningLabel,0,sizeof(TCHAR)*30);
		_stprintf(warningLabel,_T("View Warnings (%d)"),warningCount);
		SetWindowText(hwarningButton,warningLabel);
		EnableWindow(hwarningButton,FALSE);
	}
	void EnableEjectButton()
	{
		EnableWindow(hejectButton,TRUE);
	}
	void DisableStartButton()
	{
		EnableWindow(hstartButton,FALSE);
	}
#ifdef DRIVE_IMAGING
	void EnableDriveImageButton()
	{
		if(isUserAdmin)
		{
			EnableWindow(hDriveImageButton,TRUE);
		}
	}
	void DisableDriveImageButton()
	{
		EnableWindow(hDriveImageButton,FALSE);
	}
#endif
	void DisableAbortButton()
	{
		EnableWindow(habortButton,FALSE);
	}
	int Errors()
	{
		return errorCount;
	}
	int Warnings()
	{
		return warningCount;
	}

//TODO:ReleasDC when done 
};

// Definition: relative pixel = 1 pixel at 96 DPI and scaled based on actual DPI.
class CDPI
{
public:
    CDPI() : _fInitialized(false), _dpiX(96), _dpiY(96) { }
    
    // Get screen DPI.
    int GetDPIX() { _Init(); return _dpiX; }
    int GetDPIY() { _Init(); return _dpiY; }

    // Convert between raw pixels and relative pixels.
    int ScaleX(int x) { _Init(); return MulDiv(x, _dpiX, 96); }
    int ScaleY(int y) { _Init(); return MulDiv(y, _dpiY, 96); }
    int UnscaleX(int x) { _Init(); return MulDiv(x, 96, _dpiX); }
    int UnscaleY(int y) { _Init(); return MulDiv(y, 96, _dpiY); }

    // Determine the screen dimensions in relative pixels.
    int ScaledScreenWidth() { return _ScaledSystemMetricX(SM_CXSCREEN); }
    int ScaledScreenHeight() { return _ScaledSystemMetricY(SM_CYSCREEN); }

    // Scale rectangle from raw pixels to relative pixels.
    void ScaleRect(__inout RECT *pRect)
    {
        pRect->left = ScaleX(pRect->left);
        pRect->right = ScaleX(pRect->right);
        pRect->top = ScaleY(pRect->top);
        pRect->bottom = ScaleY(pRect->bottom);
    }
    // Determine if screen resolution meets minimum requirements in relative
    // pixels.
    bool IsResolutionAtLeast(int cxMin, int cyMin) 
    { 
        return (ScaledScreenWidth() >= cxMin) && (ScaledScreenHeight() >= cyMin); 
    }

    // Convert a point size (1/72 of an inch) to raw pixels.
    int PointsToPixels(int pt) { _Init(); return MulDiv(pt, _dpiY, 72); }

    // Invalidate any cached metrics.
    void Invalidate() { _fInitialized = false; }

private:
    void _Init()
    {
        if (!_fInitialized)
        {
            HDC hdc = GetDC(NULL);
            if (hdc)
            {
                _dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
                _dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
                ReleaseDC(NULL, hdc);
            }
            _fInitialized = true;
        }
    }

    int _ScaledSystemMetricX(int nIndex) 
    { 
        _Init(); 
        return MulDiv(GetSystemMetrics(nIndex), 96, _dpiX); 
    }

    int _ScaledSystemMetricY(int nIndex) 
    { 
        _Init(); 
        return MulDiv(GetSystemMetrics(nIndex), 96, _dpiY); 
    }
private:
    bool _fInitialized;

    int _dpiX;
    int _dpiY;
};


extern gui_resources gui;


	
