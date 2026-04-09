
#define _UNICODE
#define UNICODE

#include "WSTRemote.h"
#include "WSTLog.h"
#include "cfilefind.h"
#include "tstl.h"
#include "SysCallsLog.h"

using namespace std;

#define FILE_FOUND WM_APP+0x0001

void floppy_latt_check(vector<tstring> &drives,vector<tstring> &filteredDrives)
{
	
	tstring curDir,curDrive,curPath; 
	TCHAR curDirA[MAX_PATH];
	memset(curDirA,0,MAX_PATH);
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
  	GetCurrentDirectory(sizeof(curDirA),curDirA);
	curDir=tstring(curDirA);
	curPath=tstring(curDirA);
	curDrive=curDir.substr(0,curDir.find_first_of(_T("\\"))+1);


	for(vector<tstring>::iterator it=drives.begin();it<drives.end();it++)
	{
		if((!_tcscmp((*it).c_str(),_T("A:\\")))||(!_tcscmp((*it).c_str(),_T("a:\\"))))
		{   
			;
		}
		else if(!_tcsicmp((*it).c_str(),curDrive.c_str()))
		{
			;
		}
		else
		{
			filteredDrives.push_back(*it);
		}
	}
	AppLog.Write("After drive iteration",WSLMSG_STATUS);
	
}

int InitializeSPStruct(searchParams &sp,vector<tstring> drives,map<tstring,int> extensions,unsigned _int64 MaximumFileSize,int PreviousDays,tstring lattDir,int createdDate,int modifiedDate)
{

#ifndef UNICODE

	vector<wstring> wdrives;

	for(vector<string>::iterator vit=drives.begin();vit!=drives.end();vit++)
	{
		sp.drives.push_back(wstring((*vit).begin(),(*vit).end()));
	}

	wstring key;
	//Mapping from ascii to unicode
	for(map<string,int>::iterator sit=extensions.begin();sit!=extensions.end();sit++)
	{
		key=wstring((*sit).first.begin(),(*sit).first.end());
		sp.extensions[key]=(*sit).second;
	}
	//Mapping from ascii to unicode
#else
	for(vector<tstring>::iterator vit=drives.begin();vit!=drives.end();vit++)
	{
		sp.drives.push_back(*vit);
	}
	//Mapping from ascii to unicode
	for(map<tstring,int>::iterator sit=extensions.begin();sit!=extensions.end();sit++)
	{
		sp.extensions[(*sit).first]=(*sit).second;
	}

#endif
	
	sp.maxFileSize=MaximumFileSize;
	sp.previousDays=PreviousDays;
#ifndef UNICODE
	sp.lattDrive=wstring(lattDir.begin(),lattDir.end());
#else
	sp.lattDrive=lattDir;
#endif
	sp.createdDate=createdDate;
	sp.modifiedDate=modifiedDate;

	return 0;
}

struct stStruct
{
	searchParams sp;
	WSTRemote *w;
};

namespace FileSearch
{
	extern int searchMultipleTargets(searchParams,WSTRemote *,tstring);
}
//typedef int (*lpsearchfn )(searchParams,WSTRemote *);
unsigned __stdcall searchThread(void *arglist)
{

	searchParams sp;
	stStruct *st;
	st= ((stStruct *)arglist);
	sp=(st->sp);
	WSTRemote *w=st->w;
	TCHAR globalCaseDir[MAX_PATH];
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH,globalCaseDir);
	AppLog.Write("Calling searchMultipleTargets",WSLMSG_STATUS);
	FileSearch::searchMultipleTargets(sp,w,globalCaseDir);
	return 0;
}

int WSTRemote::startSearch(vector<tstring> drives,map<tstring,int> extensions)
{

	if(drives.size() == 0)
	{
		AppLog.Write("FileSearch : No drives to search ",WSLMSG_NOTE);
		return 0;
	}
	else
	{
		tstring msg=_T("Searching drives ");
		for(int i=0;i<drives.size();i++)
			msg+=drives[i];
		AppLog.Write(msg.c_str(),WSLMSG_STATUS);
	}
	if(config.createdDate == 0 && config.modifiedDate == 0)
	{
		AppLog.Write("FileSearch : No files to be searched as both modified sate and created date are empty",WSLMSG_NOTE);
		return 0;
	}
	

//Check if the latt token is included in the driveList

	vector<tstring> filteredDrives;
	floppy_latt_check(drives,filteredDrives);
	AppLog.Write("Returned from floppy_latt_check",WSLMSG_STATUS);
	TCHAR dstDirA[MAX_PATH];
	INC_SYS_CALL_COUNT(GetCurrentDirectory); // Needed to be INC TKH
	GetCurrentDirectory(MAX_PATH,dstDirA);
	tstring dstDir=tstring(dstDirA);

	searchParams sp;//structure to be initialized 
	
	dstDir+=tstring(_T("\\Files\\Collected"));
	InitializeSPStruct(sp,filteredDrives,extensions,config.getMaximumFileSize,config.getPreviousDays,dstDir,config.createdDate,config.modifiedDate);

	//Spawn the search thread
	unsigned int searchID=0;
	bool searchOver=false;
	HANDLE searchHandle=INVALID_HANDLE_VALUE;
	stStruct st;
	st.sp=sp;
	st.w=this;
	currentState=COPYING_FILES;
	filesProcessed=0;

	SYSTEMTIME cpTime;
	string msg;
	INC_SYS_CALL_COUNT(GetSystemTime); // Needed to be INC TKH
	GetSystemTime(&cpTime);
	stringstream ssTime;

	ssTime<<cpTime.wHour<<":"<<cpTime.wMinute<<":"<<cpTime.wSecond;
	msg="Start Time of Copy "+ssTime.str();
	AppLog.Write(msg.c_str(),WSLMSG_STATUS);

    searchHandle=(HANDLE) _beginthreadex(0,NULL,searchThread,(void *)(&st),0,&searchID); 

	
	while(true)
	{
		INC_SYS_CALL_COUNT(WaitForSingleObject); // Needed to be INC TKH
		DWORD retVal=WaitForSingleObject(searchHandle,INFINITE);
		switch(retVal)
		{
		case WAIT_OBJECT_0:
				searchOver=true;
				if(searchHandle != INVALID_HANDLE_VALUE)
				{
						INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
						CloseHandle(searchHandle);
						INC_SYS_CALL_COUNT(GetSystemTime); // Needed to be INC TKH
						GetSystemTime(&cpTime);
						ssTime.str("");
						//ssTime.str().clear();
						//ssTime.clear();
						ssTime<<cpTime.wHour<<":"<<cpTime.wMinute<<":"<<cpTime.wSecond;
						msg.clear();
						msg="End Time of Copy "+ssTime.str();
						AppLog.Write(msg.c_str(),WSLMSG_STATUS);
				}
				break;
		case WAIT_ABANDONED:
			if(searchHandle != INVALID_HANDLE_VALUE)
				{
					AppLog.Write("Search Thread abandoned",WSLMSG_ERROR); 
					INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
					CloseHandle(searchHandle);
				}
			break;
		case WAIT_FAILED:
			if(searchHandle != INVALID_HANDLE_VALUE)
				{
					AppLog.Write("Search Thread wait failed",WSLMSG_ERROR);
					INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
					CloseHandle(searchHandle);
				}
			break;
		
		};
		if(currentState == ABORTING || currentState == ABORTED)
			break;
		if(searchOver)
			break;				
	}
	
	gui.ResetProgressBar();
	stringstream ss;
	ss<<"Total files captured "<<filesProcessed<<" "<<currentState<<endl;
	AppLog.Write("File search complete ",WSLMSG_STATUS);
	AppLog.Write(ss.str().c_str(),WSLMSG_STATUS);
	
	if(currentState!=ABORTING)
	{
		AppLog.Write("Abort file not found",WSLMSG_STATUS);
		currentState=CAPTURE_COMPLETE;
	}
	else
	{
		AppLog.Write("Abort file found",WSLMSG_STATUS);
		
		currentState=ABORTED;
	}
	totalFileCount=filesProcessed;

	
	
	return 0;

}


#undef _UNICODE
#undef UNICODE