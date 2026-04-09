


#define _UNICODE 
#define UNICODE 

#include <iostream>
#include <process.h>

#include <string>
#include <vector>
#include <map>
#include <fstream>

#undef _UNICODE
#undef UNICODE

#include "WSTRemote.h"

#define _UNICODE 
#define UNICODE 

#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <AclAPI.h>
#include "wstlog.h"
#include "SysCallsLog.h"

FILE *fdHash;
char strbuf[1024];

#define GB4 4294967290

extern char *imageList[7];
#define LOG_MSG_BUFSIZE 2*260+2048

#ifdef HBFS
#define SIGCOUNT 87 // Update this to number of entries in the signature array
#endif

using namespace std;

#define ERR_INVALID_HANDLE -101
#define ERR_GET_FILE_TIME -102
#define ERR_SET_FILE_TIME -103


#ifdef PRO
extern TCHAR caseDrive[];
#endif
bool isImage(string extension)
{
	
	for(int i=0;i<7;i++)
	{
		if(!strnicmp(extension.c_str(),imageList[i],extension.length()))
			return true;
	}
	return false;
}


int GetFileAccessTime(TCHAR *fileName,FILETIME &accessTime)
{
	
	HANDLE hFile;
	TCHAR lattMsg[2*MAX_PATH];
	
	//Only Read required to get access times.
	 hFile = CreateFile(fileName, GENERIC_READ, 0, NULL,OPEN_EXISTING, 0, NULL);
	//hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, 0, NULL);
	
	
	if(hFile == INVALID_HANDLE_VALUE)
	{
		memset(lattMsg,0,2*MAX_PATH);
	//	_stprintf_s(lattMsg,2*MAX_PATH,_T("%s %s"),fileName,_T(":Failed to Get Access Time, CreateFile Failed, System Error: "));
	//	LOG_SYSTEM_ERROR(lattMsg);
		return ERR_INVALID_HANDLE;
	}

	if(!GetFileTime(hFile,NULL,&accessTime,NULL))
	{
		memset(lattMsg,0,2*MAX_PATH);
		_stprintf_s(lattMsg,2*MAX_PATH,_T("%s %s"),fileName,_T(":Failed to Get Access Time, GetFileTime Failed, System Error: "));
		LOG_SYSTEM_ERROR(lattMsg);
		CloseHandle(hFile);
		return ERR_GET_FILE_TIME;
	}

	if(hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
	}

	return ERROR_SUCCESS;


}


int RestoreAccessTime(TCHAR *fileName, FILETIME atime)
{
	
	HANDLE hFile;
	TCHAR lattMsg[2*MAX_PATH];
	
	 //Works to set access time only if we have write permissions if we cannot return and pop from the list anyways
	hFile = CreateFile(fileName, FILE_WRITE_ATTRIBUTES, 0, NULL,OPEN_EXISTING, FILE_WRITE_ATTRIBUTES, NULL);
	//hFile = CreateFile(fileName, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	

	if(hFile == INVALID_HANDLE_VALUE)
	{
		memset(lattMsg,0,2*MAX_PATH);
		//_stprintf_s(lattMsg,2*MAX_PATH,_T("%s %s"),fileName,_T(":Failed to Reset Access Time, CreateFile Failed"));
		//AppLog.Write(lattMsg,WSLMSG_PROF);
		return ERR_INVALID_HANDLE;
	}
	if(!SetFileTime(hFile,NULL,&atime,NULL))
	{
		memset(lattMsg,0,2*MAX_PATH);
		_stprintf_s(lattMsg,2*MAX_PATH,_T("%s %s"),fileName,_T(":Failed to Reset Access Time, SetFileTime Failed: "));
		LOG_SYSTEM_ERROR(lattMsg);
		CloseHandle(hFile);
		return ERR_SET_FILE_TIME;
	}

	if(hFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
	}

	return ERROR_SUCCESS;
}
	



std::string to_utf8(const wchar_t* buffer, int len)
{
	int errorCode =0;
	int nChars = ::WideCharToMultiByte(
		CP_UTF8,
		0,
		buffer,
		len,
		NULL,
		0,
		NULL,
		NULL);
	if (nChars == 0) return "";

	INC_SYS_CALL_COUNT(WideCharToMultiByte); // Needed to be INC TKH
	string newbuffer;
	newbuffer.resize(nChars) ;
	
	errorCode = ::WideCharToMultiByte(
 		CP_UTF8,
		0,
		buffer,
		len,
		const_cast< char* >(newbuffer.c_str()),
		nChars,
		NULL,
		NULL); 

	if(errorCode == 0)
	{
		newbuffer="XXX";
	}

	return newbuffer;
}

std::string to_utf8(const std::wstring& str)
{
	return to_utf8(str.c_str(), (int)str.size());
}


wstring escapeXML(wstring str)
{
	TCHAR findStr[]={'&','<','>','\0'};
	wstring escapedString=str;
	int index=0;
	int jump=0;

	while(((index=escapedString.find_first_of(findStr,jump))!=escapedString.npos))
	{
		wchar_t c=(escapedString.c_str())[index];
		jump=0;

		if(c==L'&')
		{
			escapedString = escapedString.substr(0,index)+L"&amp;"+escapedString.substr(index+1);
			jump=index+4;
		}
		else if(c==L'<')
		{
			escapedString = escapedString.substr(0,index)+L"&lt;"+escapedString.substr(index+1);
			jump=index+3;
		}
		else if(c==L'>')
		{
			escapedString = escapedString.substr(0,index)+L"&gt;"+escapedString.substr(index+1);
			jump=index+3;
		}

	}
	return escapedString;

}

static const size_t MD5HashByteCount = 16;
using namespace std;

WSTRemote *rAgent;
int abortFlag=0;
typedef struct searchParams
{
	vector<wstring> drives;
	map<wstring,int> extensions;
	wstring lattDrive;
	unsigned long long maxFileSize;
	int previousDays;
	int createdDate;
	int modifiedDate;



	~searchParams(void)
	{

		if(drives.size()!=0)
			drives.clear();

		/*	   
		if(extensions.size()!=0)
		extensions.clear();
		*/
		
		lattDrive.clear();
	}




}searchParams;


vector<wstring> extensionList;
unsigned long long maxFileSize;
int previousDays;
SYSTEMTIME acqTime;
int stcount;
DWORD parentThreadId;

#define FILE_FOUND WM_APP+0x0001

HANDLE copyMutex;
HANDLE abortMutex;
vector<wstring> filesToCopy;
vector<FILETIME> fileTimes;


/*FileHash Code */

#ifdef PRO
extern TCHAR uslattDrive[];
#endif
int MD5Hash(wstring filename, BYTE *hashVal)
{
	int iRetVal = 0;
	FILE *pFile;
	BYTE buf[8192];
	BYTE hv[MD5HashByteCount];
	MD5_CTX md5Data;
	int iBytesRead;
	bool bEndOfFile=false;
	errno_t err;

	MD5Init(&md5Data);
#ifdef PRO
	TCHAR path_to_id_file[40];
	memset(path_to_id_file,0,40*sizeof(TCHAR));
	_tcsncpy_s(path_to_id_file,40,uslattDrive,4);
	_tcsncat(path_to_id_file,_T(".id"),3);
	err = _tfopen_s(&pFile, path_to_id_file, TEXT("r"));
#else
	err = _tfopen_s(&pFile, TEXT("\\.id"), TEXT("r"));
#endif
	if(pFile)
	{
		iBytesRead = (int)fread(buf, 1, 50, pFile);
		MD5Update(&md5Data, buf, iBytesRead);
		fclose(pFile);
	}

	err = _tfopen_s(&pFile, filename.c_str(), TEXT("rb"));
	if (pFile)
	{
		while (!bEndOfFile)
		{
			iBytesRead = (int)fread(buf, 1, 8192, pFile);
			if (iBytesRead == 0)
			{
				bEndOfFile = true;
			}
			else
			{
				MD5Update(&md5Data, buf, iBytesRead);
			}
		}
		MD5Final(hv, &md5Data);

		// returns hash value as byte array
		for (int i = 0; i < 16; ++i)
			hashVal[i] = hv[i];

		fclose(pFile);
	}
	return iRetVal;
}



void CloseFileHandles(FILE *fdh)
{

	if(fdh)
	{
		fclose(fdh);
		fdh=NULL;
	}
}


namespace FileSearch
{

#ifdef HBFS
	char *signature[]=
                {
					//Often searched should come first (speed)
					"com,exe,dll,drv,sys,obj","2","\x4D\x5A",
					"jpg,jpeg,jfif,jpe","3","\xFF\xD8\xFF",
					"doc,ppt,xls","8","\xD0\xCF\x11\xE0\xA1\xB1\x1A\xE1",
					"docx,pptx,xlsx","4","\x50\x4B\x03\x04",
					"pdf","4","\x25\x50\x44\x46",
                    //Images
					"png","8","\x89\x50\x4E\x47\x0D\x0A\x1A\x0A",
					"tiff,tif","3","\x49\x20\x49",
					"bmp","2","\x42\x4D",
					"gif","6","\x47\x49\x46\x38\x37\x61",
					"psd","4","\x38\x42\x50\x53",
					//audio and videos
					"mp3","3","\x49\x44\x33",  
					"flv","4","\x46\x4C\x56\x01",
					"aiff","5","\x46\x4F\x52\x4D\x00",
					"wav","4","\x52\x49\x46\x46",
					"mov","4","\x6D\x6F\x6F\x76",
					"asf,wma,wmv","8","\x30\x26\xB2\x75\x8E\x66\xCF\x11",
					"ra","9","\x2E\x52\x4D\x46\x00\x00\x00\x12\x00",
					"ogg","6","\x4F\x67\x67\x53\x00\x02",
					//System files
					"class","4","\xCA\xFE\xBA\xBE",
					"jar","10","\x50\x4B\x03\x04\x14\x00\x08\x00\x08\x00",
					//Compression
					"gz,tgz,gzip","2","\x1F\x8B\x08",
					"tar","2","\x1F\x9D",
					"7z","6","\x37\x7A\xBC\xAF\x27\x1C",
					"bz2","3","\x42\x5A\x68",
					"zip","4","\x50\x4B\x03\x04",
					"rar","7","\x52\x61\x72\x21\x1A\x07\x00",
					//Documents
					
					//Email
					"pst","4","\x21\x42\x44\x4E",
					"dbx","4","\xCF\xAD\x12\xFE",
					"msf","16","\x2F\x2F\x20\x3C\x21\x2D\x2D\x20\x3C\x6D\x64\x62\x3A\x6D\x6F\x72"

	};



	
bool isInSignatureArray(char *signature)
{
	int offset=0;
	char *ext;
	char sig[20],signatureList[50];
	char *sep =",";//Separator in the signature list 
	vector<wstring>::iterator ext_it;


	memset(signatureList,0,50);
	strncpy(signatureList,signature,strlen(signature));

	if(signature == NULL)
	{
		return false;
	}
	else
	{
		for(ext_it=extensionList.begin();ext_it != extensionList.end();ext_it++)
		{	
			ext = strtok(signatureList,sep); //restart for every 
			
			memset(sig,0,20);
			wcstombs(sig,(*ext_it).c_str(),wcslen((*ext_it).c_str()));
			while(ext != NULL)
			{
				if(!strncmp(sig,ext,strlen(ext)))
					return true;
				ext = strtok(NULL,",");
			}
		}
	}
	return false;
}


char *CheckFileHeaderForMatch(wstring filename)
{
    FILE *ifd;
	char *val;
    int i=0,size;
    char tok[10];
	wchar_t tempBuf[MAX_PATH],extW[20];

	ifd=_wfopen(filename.c_str(),L"r");
	
    if(ifd != NULL)
    {
        for(i=2;i<SIGCOUNT;i+=3)
        {
			memset(tempBuf,0,MAX_PATH);
			memset(extW,0,20);
			mbstowcs(extW,signature[i-2],20);

			_stprintf(tempBuf,L"extension type %s checked for file %s",extW,filename.c_str());
			AppLog.Write(tempBuf,WSLMSG_DEBUG);

			size=atoi(signature[i-1]);
			val= (char*)malloc(size*sizeof(char));
			fread(val,size,1,ifd);

			if(!_strnicmp(val,signature[i],size))
			{
				_stprintf(tempBuf,L"file %s is a %s type",extW,filename.c_str());
				AppLog.Write(tempBuf,WSLMSG_DEBUG);
				free(val);
				fclose(ifd);
				return signature[i-2];
			}
			else
			{
				free(val);
				fseek(ifd,0,SEEK_SET);
			}
		}
	}
	return NULL;
}
#endif	
int DaysElapsed(FILETIME fileTime)
	{
		//Current Time into cu
		SYSTEMTIME stc;
		INC_SYS_CALL_COUNT(GetSystemTime); // Needed to be INC TKH
		GetSystemTime(&stc);
		FILETIME ftc;
		INC_SYS_CALL_COUNT(FileTimeToSystemTime); // Needed to be INC TKH
		SystemTimeToFileTime(&stc,&ftc);

		ULARGE_INTEGER cu;
		cu.HighPart=ftc.dwHighDateTime;
		cu.LowPart=ftc.dwLowDateTime;

		//fileTime into fu
		ULARGE_INTEGER fu;
		fu.HighPart=fileTime.dwHighDateTime;
		fu.LowPart=fileTime.dwLowDateTime;

		if(cu.QuadPart < fu.QuadPart)
		{
			return -1;
		}
		DWORD seconds = ((cu.QuadPart-fu.QuadPart)/pow(10.0,7));//100 nanonsecond intervals
		DWORD hours = seconds/3600;
		int days = hours/24;
		return days;
		
	}

	struct fileSearch
	{
		wstring drive;
		wstring extension;
		int createdDate;
		int modifiedDate;

	};

	void Recurse(wstring,wstring,int,int);

	bool Aborting()
	{
		FILE *fd;
		if((fd=fopen("Abort.WST","r"))!=NULL)
		{
			abortFlag=1;
			fclose(fd);
			rAgent->currentState=ABORTING;//Very important to let remote know that we are aborting
			return true;
		}
		return false;
	}


	unsigned __stdcall Thread(void *arglist)
	{

		fileSearch *pattern=(fileSearch *)arglist;
		Recurse(pattern->drive,pattern->extension,pattern->createdDate,pattern->modifiedDate);
		fflush(stdout);
		_endthreadex(0);
		return 0;

	}

	//Copy constraint functions

	//1. File type

	bool inExtensionList(wstring filename)
	{
		
		for(vector<wstring>::iterator it=extensionList.begin();it!=extensionList.end();it++)
		{
			wstring ext((*it).begin(),(*it).end());
			if(!(_tcsicmp(ext.c_str(),(filename.substr(filename.find_last_of(L'.')+1)).c_str())))
			{
				return true;
			}
		}
		return false;
	}

	//2. File size
	bool isWithinFileSize(unsigned long long fileSize)
	{
		if(fileSize < 0)
			return -2;	// Invalid file size
		else if(fileSize > GB4)
			return -1;	// Larger than FAT32 4GB limit 
		else if(fileSize > maxFileSize)
			return 0;	// Larger than user specified limit - ignore
		else
			return 1;	// ok
	}

	//time check
	BOOL dateCheck(FILETIME ftime)
	{
		SYSTEMTIME fileTime;
		INC_SYS_CALL_COUNT(FileTimeToSystemTime); // Needed to be INC TKH
		FileTimeToSystemTime(&ftime,&fileTime);
		int daysElapsed = DaysElapsed(ftime);
		if((daysElapsed <= previousDays)&&(daysElapsed >= 0))
		{
			return true;
		}
		
		return FALSE;
	}

	//3. File time (Created and/or Last written)
	bool isWithinLastPreviousDays(FILETIME ftime,FILETIME ctime,int createdDate,int modifiedDate)
	{
		//0--Both,1--createdDate,2--modifiedDate. Can never be 0,0
		int dc=(createdDate^modifiedDate)?(createdDate==0?2:1):0;
		switch(dc)
		{
		case 0:
			return (dateCheck(ftime)||dateCheck(ctime));
		case 1:
			return (dateCheck(ctime) ? true : false);
		case 2:
			return (dateCheck(ftime) ? true : false);
		}
		return false;
	}

	
	wstring GetOwnerInfo(wstring filename)
	{

		PSID owner=NULL;
		PSECURITY_DESCRIPTOR psd=NULL;
		DWORD retval;

		HANDLE hFile=NULL;

		INC_SYS_CALL_COUNT(CreateFile); // Needed to be INC TKH
		hFile=CreateFile(filename.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

		if(hFile==INVALID_HANDLE_VALUE)
		{
			memset(strbuf,0,1024);
			sprintf_s(strbuf,sizeof(strbuf),"%s %d","GetOwnerInfo ",GetLastError());
			AppLog.Write(strbuf,WSLMSG_NOTE);
			return L"";
		}

		retval=GetSecurityInfo(hFile,SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION,&owner,NULL,NULL,NULL,&psd);

		if(retval != ERROR_SUCCESS)
		{
			memset(strbuf,0,1024);
			sprintf_s(strbuf,sizeof(strbuf),"%s %d","Could not capture Owner information GetNamedSecurityInfo ",GetLastError());
			AppLog.Write(strbuf,WSLMSG_NOTE);
			CloseHandle(hFile);
			return L"";
		}
		else
		{
			TCHAR fileOwner[MAX_PATH];
			memset(fileOwner,0,MAX_PATH);
			TCHAR dname[MAX_PATH];
			memset(dname,0,MAX_PATH);
			SID_NAME_USE su;
			DWORD length=MAX_PATH;
			DWORD dlength=MAX_PATH;

			INC_SYS_CALL_COUNT(LookupAccountSid); // Needed to be INC TKH
			if(!LookupAccountSid(NULL,owner,fileOwner,&length,dname,&dlength,&su))
			{
				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"%s %d","Could not capture Owner information [LookupAccountSid ",GetLastError());
				AppLog.Write(strbuf,WSLMSG_DEBUG);

				INC_SYS_CALL_COUNT(LocalFree); // Needed to be INC TKH
				LocalFree(psd);
				CloseHandle(hFile);
				return L"";
			}
			else
			{
				INC_SYS_CALL_COUNT(LocalFree); // Needed to be INC TKH
				LocalFree(psd);
				CloseHandle(hFile);
				return wstring(fileOwner);
			}

		}

		CloseHandle(hFile);


	}

	//Retrieve the file attributes

	DWORD AttrList[]={FILE_ATTRIBUTE_READONLY,FILE_ATTRIBUTE_HIDDEN,FILE_ATTRIBUTE_ARCHIVE};
	char *AttrValue[]={"READONLY","HIDDEN","ARCHIVE"};
	

	DWORD GetAttr(wstring fileName)
	{
		DWORD fileattr=0;
		fileattr=GetFileAttributes(fileName.c_str());

		for(int i=0;i<(sizeof(AttrList)/sizeof(DWORD));i++)
		{
			fprintf_s(fdHash,"<%s>%s</%s>\n",AttrValue[i],(((fileattr&AttrList[i])!=0)?"TRUE":"FALSE"),AttrValue[i]);
		}
		return 0;
	}



	

	


	void Recurse(wstring drive,wstring extension,int createdDate,int modifiedDate)
	{

		WIN32_FIND_DATA w32data; //IN Remove MFC
		wstring currRecurseDir = drive; //IN Remove MFC
		
		wstring pattern=(drive+L"\\*"+extension).c_str();

		HANDLE findFirst=FindFirstFile(pattern.c_str(),&w32data); //IN Remove MFC

		
		
		//Must change signature of this function to return error values TODO
		if(findFirst == INVALID_HANDLE_VALUE)
		{
			return;
		}
		
		
		DWORD waitResult;
		DWORD abortMutexRet;

		
		do
		{
			gui.DisplayResult(_T("File Capture: Scanning ..."));
			SendMessage(gui.hProgress, PBM_STEPIT, 0, 0); 

			INC_SYS_CALL_COUNT(WaitForSingleObject); // Needed to be INC TKH
			abortMutexRet=WaitForSingleObject(abortMutex,INFINITE);
			switch(abortMutexRet)
			{	
			case WAIT_OBJECT_0:
				if(Aborting())
				{							

					if(!ReleaseMutex(abortMutex))
					{
						memset(strbuf,0,1024);
						sprintf_s(strbuf,sizeof(strbuf),"%s %d","Recurse :Error releasing abortMutex mutex",GetLastError());
						AppLog.Write(strbuf,WSLMSG_ERROR);

						_endthreadex(-1);
					}
					else
					{
						_endthreadex(0);
					}
				}
				else
				{
					if(!ReleaseMutex(abortMutex))
					{

						
						memset(strbuf,0,1024);
						sprintf_s(strbuf,sizeof(strbuf),"%s %d","Recurse :Error releasing abortMutex mutex",GetLastError());
						AppLog.Write(strbuf,WSLMSG_ERROR);
						
					}
				}

				break;
			case WAIT_ABANDONED:

				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"%s %d","Recurse :Error abortMutex Abandoned",GetLastError());
				AppLog.Write(strbuf,WSLMSG_ERROR);


				break;
			case WAIT_FAILED:
				
				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"%s %d","Recurse :Error abortMutex failed",GetLastError());
				AppLog.Write(strbuf,WSLMSG_ERROR);
			}

			if( (wcslen(w32data.cFileName) == 1)  && (wcsncmp(w32data.cFileName,L".",1) == 0) ) continue;
			if( (wcslen(w32data.cFileName) == 2)  && (wcsncmp(w32data.cFileName,L"..",2) == 0) ) continue;

			if((w32data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
			{
				
				wstring dir=currRecurseDir + L"\\" + w32data.cFileName;
				////Remove TODO
				//TCHAR msg[MAX_PATH*2];
				//ZeroMemory(msg,MAX_PATH*2);
				//_stprintf(msg,_T("Recurse directory for next iteration %s"),dir.c_str());
				//AppLog.Write(msg,WSLMSG_DEBUG);
				////Remove TODO

				//This is good place to save and restore Access Time for the directory. 
				//The access time should be restored only after all the operation with the current directory is complete
				//The variable daccessTime is local and will be the relevant one in the stack frame
			//	FILETIME daccessTime=w32data.ftLastAccessTime;

				//SYSTEMTIME staccess;
				//FileTimeToSystemTime(&w32data.ftLastAccessTime,&staccess);

				Recurse(dir,extension,createdDate,modifiedDate);

			//	RestoreAccessTime(const_cast<TCHAR *>(dir.c_str()),daccessTime,1);//Restore access time of the directory

			}
			/* Remove MFC IN*/
			else
			{
				
				
				wstring fileName=w32data.cFileName;//Remove MFC IN

				unsigned long long fileSize = (w32data.nFileSizeHigh * (MAXDWORD+1)) + w32data.nFileSizeLow;//Remove MFC IN
				FILETIME fileWriteTime;
				FILETIME createdTime;
				FILETIME accessTime;
				memset(&accessTime,0,sizeof(FILETIME));
				
				fileWriteTime = w32data.ftLastWriteTime;
				createdTime   = w32data.ftCreationTime;
				accessTime    = w32data.ftLastAccessTime;
				
				
#ifdef HBFS
				wstring filePath = currRecurseDir + L"\\" + w32data.cFileName; //Remove MFC IN
				if(isWithinFileSize(fileSize) && isWithinLastPreviousDays(fileWriteTime,createdTime,createdDate,modifiedDate) && ( inExtensionList(fileName) || isInSignatureArray(CheckFileHeaderForMatch(filePath)) ) )
#else
				if( inExtensionList(fileName)  && isWithinLastPreviousDays(fileWriteTime,createdTime,createdDate,modifiedDate))
#endif
				{
					wstring filepath;
					filepath = currRecurseDir + L"\\" + w32data.cFileName; //Remove MFC IN
					
					wchar_t FileSizeMessage[MAX_PATH+1+100];
					memset(FileSizeMessage,0,(MAX_PATH+1+100)*sizeof(wchar_t));

					int retValidSize = isWithinFileSize(fileSize);

					if(retValidSize < 1) // Not within File Size
					{
						wcscpy_s(FileSizeMessage,MAX_PATH+100, filepath.c_str());
						if(retValidSize == -2) // Invalid file size (Negative size).
						{
							wcscat_s(FileSizeMessage,MAX_PATH+100,L" cannot be copied due to invalid file size.");
							AppLog.Write(FileSizeMessage,WSLMSG_ERROR);
							gui.DisplayWarning(FileSizeMessage);
						}

						if(retValidSize == -1) // Larger than FAT32 4GB limit
						{
							wcscat_s(FileSizeMessage,MAX_PATH+100,L" is more than 4GB, cannot be copied due to FAT32 file size constraints.");
							AppLog.Write(FileSizeMessage,WSLMSG_PROF);
							gui.DisplayWarning(FileSizeMessage);
						}
		
						AppLog.Write("File size not within the specified limits",WSLMSG_DEBUG);

						RestoreAccessTime(const_cast<TCHAR *>(filepath.c_str()),accessTime);
					}
					else
					{
						INC_SYS_CALL_COUNT(WaitForSingleObject); // Needed to be INC TKH
						waitResult=WaitForSingleObject(copyMutex,INFINITE);


						switch(waitResult)
						{
						case WAIT_OBJECT_0:
							{


								filesToCopy.push_back(filepath);
								fileTimes.push_back(accessTime);
								//PostThreadMessage(parentThreadId,WM_USER+0x0000,0L,0L);
							}
							{
								if(!ReleaseMutex(copyMutex))
								{

									memset(strbuf,0,1024);
									sprintf_s(strbuf,sizeof(strbuf),"%s %d","Recurse :Error releasing copyMutex mutex",GetLastError());
									AppLog.Write(strbuf,WSLMSG_ERROR);
									_endthreadex(-1);
								}
							}
							break;
						case WAIT_ABANDONED:


							memset(strbuf,0,1024);
							sprintf_s(strbuf,sizeof(strbuf),"%s %d","Recurse :Error copyMutex Abandoned",GetLastError());
							AppLog.Write(strbuf,WSLMSG_ERROR);
							_endthreadex(-1);
							return;

						case WAIT_FAILED:

							memset(strbuf,0,1024);
							sprintf_s(strbuf,sizeof(strbuf),"%s %d","Recurse :Error copyMutex Failed",GetLastError());
							AppLog.Write(strbuf,WSLMSG_ERROR);


							_endthreadex(-1);
							return;
						}
					}
				}
				else//Restore access time and continue
				{
					wstring fpath = currRecurseDir + L"\\" + w32data.cFileName;
					RestoreAccessTime(const_cast<TCHAR *>(fpath.c_str()),accessTime);
				}

			}
		}while(FindNextFile(findFirst,&w32data));
	}

	extern bool bmemWatermarkBreached;//IFCAP_CHANGES
	//IFCAP_CHANGES callback routine during copy
	DWORD CALLBACK CopyProgressRoutine(
		__in      LARGE_INTEGER TotalFileSize,
		__in      LARGE_INTEGER TotalBytesTransferred,
		__in      LARGE_INTEGER StreamSize,
		__in      LARGE_INTEGER StreamBytesTransferred,
		__in      DWORD dwStreamNumber,
		__in      DWORD dwCallbackReason,
		__in      HANDLE hSourceFile,
		__in      HANDLE hDestinationFile,
		__in_opt  LPVOID lpData
		)
	{
		ULONGLONG availSpace;
#ifdef PRO
		GetDiskFreeSpaceEx(caseDrive,(PULARGE_INTEGER)&availSpace,NULL,NULL);
#else
		GetDiskFreeSpaceEx(NULL,(PULARGE_INTEGER)&availSpace,NULL,NULL);
#endif
#ifdef _DEBUG
		printf("total file size %lld total bytes transferred %lld total available space %lld\n",TotalFileSize.QuadPart,TotalBytesTransferred.QuadPart,availSpace-TotalBytesTransferred.QuadPart);
#endif
		if((availSpace - TotalFileSize.QuadPart) <= MINIMUM_SPACE_REQUIRED)
		{
			*((BOOL *)(lpData))=TRUE;
			return PROGRESS_STOP;
		}
		return PROGRESS_CONTINUE;
	}


	//Return the destination directory after creating it.
	wstring mkdirp(wstring srcFileName,wstring dstDrive)
	{
		int index,cIndex;

		wstring tempFileName=srcFileName;
		wstring dirName=dstDrive;

		while((index=tempFileName.find_first_of(L"\\"))!=wstring::npos)
		{
			wstring newDir=tempFileName.substr(0,index);

			if((cIndex=newDir.find_first_of(L":"))!=wstring::npos)
			{
				newDir=newDir.substr(0,cIndex);
			}
			dirName = dirName +L"\\"+newDir;
			tempFileName=tempFileName.substr(index+1);			
		}	
		return dirName+L"\\"+tempFileName;

	}



/* //Returns Modified/Acesss/Created Time 	
BOOL RetrieveFileTimes(TCHAR *fileName, LPTSTR writeTime, LPTSTR accTime, LPTSTR createTime,int dwSize)
	{
		FILETIME ftCreate, ftAccess, ftWrite;
		SYSTEMTIME stwUTC,staUTC, stcUTC, stLocal;
		DWORD dwRet;
		HANDLE hFile;
		TCHAR Msg[2048];
		memset(Msg,0,2048);

		hFile = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL,OPEN_EXISTING, 0, NULL);

		if(hFile == INVALID_HANDLE_VALUE)
		{
			_stprintf_s(Msg,2048,_T("%s %s :Error %d"),_T("CreateFile failed in RetrieveFileTimes with filename "),fileName,GetLastError());
			AppLog.Write(Msg,WSLMSG_ERROR);
			//printf("CreateFile failed with %d\n", GetLastError());
			return FALSE;
		}

		// Retrieve the file times for the file.
		if (!GetFileTime(hFile, &ftCreate, &ftAccess, &ftWrite))
		{
			AppLog.Write("GetFileTime failed in RetrieveFileTimes",WSLMSG_ERROR);
			CloseHandle(hFile);
			return FALSE;
		}

		// Convert to local time.
		FileTimeToSystemTime(&ftWrite,  &stwUTC);
		FileTimeToSystemTime(&ftAccess, &staUTC);
		FileTimeToSystemTime(&ftCreate, &stcUTC);
		//SystemTimeToTzSpecificLocalTime(NULL, &stUTC, &stLocal);

		// Build a strings showing the date and time.
		StringCchPrintf(writeTime, dwSize, TEXT("%02d/%02d/%d  %02d:%02d"),stwUTC.wMonth, stwUTC.wDay, stwUTC.wYear,stwUTC.wHour, stwUTC.wMinute);
		StringCchPrintf(accTime, dwSize, TEXT("%02d/%02d/%d  %02d:%02d"),staUTC.wMonth, staUTC.wDay, staUTC.wYear,staUTC.wHour, staUTC.wMinute);
		StringCchPrintf(createTime, dwSize, TEXT("%02d/%02d/%d  %02d:%02d"),stcUTC.wMonth, stcUTC.wDay, stcUTC.wYear,stcUTC.wHour, stcUTC.wMinute);

		CloseHandle(hFile);
		return TRUE;
	}
	*/
	unsigned __stdcall copyThread(void *arglist)
	{


		wstring *dstdir=0;
		dstdir=(wstring *)arglist;

		DWORD mutexState;
		MSG msg;
		PeekMessage(&msg,NULL,WM_USER,WM_USER,PM_NOREMOVE);

		/*
		Comment these set of changes as IFCAP_CHANGES (Intelligent FileCapture Changes) for easier searching 
		New additions to perform intelligent copy transaction
			1. Add a local flag to the copy thread.
			2. Replace Copy with CopyFileEx and callback CopyProgressRoutine passing in the local flag to be set.
			3. In the CopyProgressRoutine check if we have breached the low watermark level in MINIMUM_SPACE_REQUIRED if so set the local flag passed in.
			4. Before performing any further copies ensure that the local flag is not set. If set switch to capture filename mode and record only file information and MD5 hash.
		--RJM
		*/

		
		INC_SYS_CALL_COUNT(WaitForSingleObject); // Needed to be INC TKH
		DWORD abortMutexRet=WaitForSingleObject(abortMutex,INFINITE);
		switch(abortMutexRet)
		{	
		case WAIT_OBJECT_0:
			if(Aborting())
			{
				if(!ReleaseMutex(abortMutex))
				{
				
					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"%s %d","copyThread :Error releasing abortMutex mutex",GetLastError());
					AppLog.Write(strbuf,WSLMSG_ERROR);
					filesToCopy.clear();
					_endthreadex(-1);
				}
				else
				{
					filesToCopy.clear();
					_endthreadex(0);
				}
			}
			else
			{
				if(!ReleaseMutex(abortMutex))
				{
					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"%s %d","copyThread :Error abortMutex mutex",GetLastError());
					AppLog.Write(strbuf,WSLMSG_ERROR);

				}
			}
			break;
		case WAIT_ABANDONED:
					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"%s %d","copyThread :Error abortMutex abandoned",GetLastError());
					AppLog.Write(strbuf,WSLMSG_ERROR);
			break;
		case WAIT_FAILED:
					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"%s %d","copyThread :Error abortMutex failed",GetLastError());
					AppLog.Write(strbuf,WSLMSG_ERROR);
			break;

		}
		while(stcount||filesToCopy.size())
		{
			INC_SYS_CALL_COUNT(WaitForSingleObject); // Needed to be INC TKH
			mutexState=WaitForSingleObject(copyMutex,INFINITE);
			switch(mutexState)
			{
			case WAIT_OBJECT_0:
				if(filesToCopy.size()==0)
				{
					if(!ReleaseMutex(copyMutex))
					{
						memset(strbuf,0,1024);
						sprintf_s(strbuf,sizeof(strbuf),"%s %d","copyThread :Error releasing copyMutex mutex",GetLastError());
						AppLog.Write(strbuf,WSLMSG_ERROR);
						_endthreadex(-1);
					}
					break;
				}
				else
				{
					try
					{
						FILETIME accessTime;
						memset(&accessTime,0,sizeof(FILETIME));
						wstring srcFileName=filesToCopy.back();
						accessTime=fileTimes.back();

						//Filenames in the format File00001 - File65530

						ostringstream ss;
						string s="00000";
						ss.str("");


						static int index=1;
						if(index<=(pow(2.0,16.0)-5))
						{
							ss<<s.substr(0,s.length()-((((int)(log((double)(index))/log(10.0)))+1)))<<index++;
						}
						else
						{
							_endthreadex(-1);//End copy thread as we don't copy any more files.
						}
						ostringstream dstFileName;
						dstFileName.clear();
						wstring extension=srcFileName.substr(srcFileName.find_last_of(L'.')+1);
						string extensionA=string(extension.begin(),extension.end());
						dstFileName<<"Files\\Collected\\"<<"File"<<ss.str().c_str()<<"."<<extensionA;

						string baseName;//For the xml file
						baseName="File"+ss.str()+"."+extensionA;

						wstring dstPath=mkdirp(srcFileName,*dstdir);


						if(dstFileName.str() == " ")
						{
							break;
						}	
						rAgent->currentState=CAPTURING_RECENT_FILES;

						
						rAgent->fileName="FILECOPIED";//For the PipeCommandHandler.
						rAgent->filesProcessed++;
						
						filesToCopy.pop_back();
						fileTimes.pop_back();


						unsigned char md5hash[20];
						memset(md5hash,0,20);

						wstring dstFileNameW;
						string dstFileNameA=string(const_cast<char *>(dstFileName.str().c_str()));
						dstFileNameW=wstring(dstFileNameA.begin(),dstFileNameA.end());

						
						
						
						memset(strbuf,0,1024);
						sprintf_s(strbuf,sizeof(strbuf),"Copying [Destination Source] %s %s",to_utf8(dstFileNameW).c_str(),to_utf8(escapeXML(srcFileName)).c_str());
						AppLog.Write(strbuf,WSLMSG_STATUS);


						MD5Hash(srcFileName.c_str(),md5hash);

						if(isImage(extensionA))
						{
							_ftprintf_s(fdHash,_T("<File type=\"image\">\n"));
						}
						else
						{
							_ftprintf_s(fdHash,_T("<File>\n"));
						}
						
						
						fprintf_s(fdHash,"<CopiedFileName> %s </CopiedFileName>\n",baseName.c_str());
						fprintf_s(fdHash,"<OriginalFileName> %s </OriginalFileName>\n",to_utf8(escapeXML(srcFileName).c_str()).c_str());
						_ftprintf_s(fdHash,_T("<Hash>"));
						for (int i = 0; i < 16; ++i)
						{
							fprintf_s(fdHash,"%02X",md5hash[i]);
						}
						_ftprintf_s(fdHash,_T("</Hash>\n"));
						/*stat information of the acquired files */	

						TCHAR accTime[100]; 
						
						SYSTEMTIME saccessTime;
						FileTimeToSystemTime(&accessTime,&saccessTime);
						StringCchPrintf(accTime, 100, TEXT("%02d/%02d/%d  %02d:%02d"),saccessTime.wMonth, saccessTime.wDay, saccessTime.wYear,saccessTime.wHour, saccessTime.wMinute);
					
						
						struct __stat64 finfo;
						int result=_wstat64(srcFileName.c_str(),&finfo);
						if(result == 0)
						{
							wstring owner;
							owner.clear();

							fprintf_s(fdHash,"<FileSize> %ld Bytes </FileSize>\n",finfo.st_size);
							
							fprintf_s(fdHash,"<ModifiedTime> %s </ModifiedTime>\n",_ctime64(&(finfo.st_mtime)));
							fprintf_s(fdHash,"<CreatedTime> %s </CreatedTime>\n",_ctime64(&(finfo.st_ctime)));
							fprintf_s(fdHash,"<AccessTime> %s </AccessTime>\n",to_utf8(accTime,100).c_str()); //This access date is obtained when 
							
							owner=GetOwnerInfo(srcFileName);
							GetAttr(srcFileName);
							if(owner != L"")
							{
								fprintf_s(fdHash,"<Owner> %s </Owner>\n",to_utf8(owner).c_str());
							}
							
							
						}
						else
						{
							memset(strbuf,0,1024);
							sprintf_s(strbuf,sizeof(strbuf),"Could not stat file %s %d",(dstFileName.str().c_str()),GetLastError());
							AppLog.Write(strbuf,WSLMSG_ERROR);
						}


						if(!bmemWatermarkBreached)
						{
							gui.DisplayResult(_T("File Capture: Copying File ..."));

							memset(gui.largeUpdateMsg,0,sizeof(TCHAR) *(LARGE_MSG_SIZE));
							_stprintf(gui.largeUpdateMsg,_TEXT("%s %s"),TEXT("Copying File: "),(srcFileName.c_str()));
							gui.DisplayStatus(gui.largeUpdateMsg);

							
							INC_SYS_CALL_COUNT(CopyFile); // Needed to be INC TKH
							if(!CopyFileEx(srcFileName.c_str(),dstFileNameW.c_str(),CopyProgressRoutine,LPVOID(&bmemWatermarkBreached),FALSE,COPY_FILE_RESTARTABLE)) //IFCAP_CHANGES
							{
								
								memset(strbuf,0,1024);
								if(GetLastError() == ERROR_REQUEST_ABORTED)//IFCAP_CHANGES
								{	
									gui.DisplayWarning(_T("Could not copy file due to insufficient space"));
									sprintf_s(strbuf,sizeof(strbuf),"Metadata information for %s collected [File not copied due to abort message]",to_utf8(escapeXML(srcFileName)).c_str());
									AppLog.Write(strbuf,WSLMSG_STATUS);
								}
								else
								{
									gui.DisplayError(_T("Could not copy file."));
									//sprintf_s(strbuf,sizeof(strbuf),"Copy thread: Error Could not copy File ");
									TCHAR lattMsg[2*MAX_PATH];
									memset(lattMsg,0,2*MAX_PATH);
									_stprintf_s(lattMsg,2*MAX_PATH,_T("%s %s"),_T("Could not copy File "),srcFileName.c_str());
									LOG_SYSTEM_ERROR(lattMsg);
								}
							}


						}
						else
						{
							fprintf_s(fdHash,"<CopyStatus> %s </CopyStatus>\n","NOT COPIED");
							memset(strbuf,0,1024);
							sprintf_s(strbuf,sizeof(strbuf),"Metadata information for %s collected [File not copied due to insufficient space on the device]",to_utf8(escapeXML(srcFileName)).c_str());
							AppLog.Write(strbuf,WSLMSG_STATUS);
						}
						//We try and restore the access times for the file at this point.
						RestoreAccessTime(const_cast<TCHAR *>(srcFileName.c_str()),accessTime);

						if(!ReleaseMutex(copyMutex))
						{
							memset(strbuf,0,1024);
							sprintf_s(strbuf,sizeof(strbuf),"copyThread: Error releasing mutex in copy %d",GetLastError());
							AppLog.Write(strbuf,WSLMSG_ERROR);
							_endthreadex(-1);
						}
							/* stat information of the acquired files */
						
						_ftprintf_s(fdHash,_T("</File>\n"));//Moved IFCAP_CHANGES
					}
					catch(...)
					{
						if(!ReleaseMutex(copyMutex))
						{
							memset(strbuf,0,1024);
							sprintf_s(strbuf,sizeof(strbuf),"copyThread: Error releasing mutex in copy %d",GetLastError());
							AppLog.Write(strbuf,WSLMSG_ERROR);

							_endthreadex(-1);
						}
					}
					break;
				}
			case WAIT_ABANDONED:
				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"copyThread: Error in copy mutex abandoned %d",GetLastError());
				AppLog.Write(strbuf,WSLMSG_ERROR);
				_endthreadex(-1);
				return 1;
			case WAIT_FAILED:
				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"copyThread: Error wait failed on mutex %d",GetLastError());
				AppLog.Write(strbuf,WSLMSG_ERROR);
				_endthreadex(-1);
				return 2;


			}
		}

		memset(strbuf,0,1024);
		sprintf_s(strbuf,sizeof(strbuf),"Finished copying files");
		AppLog.Write(strbuf,WSLMSG_STATUS);
		_endthreadex(0);
		return 0;
	}


	int searchMultipleTargets(searchParams sp,WSTRemote *w,tstring globalCaseDir)
	{

		gui.SetPercentStatusBar(10,0,100);
		AppLog.Write(globalCaseDir.c_str(),WSLMSG_DEBUG);

		DWORD exitCode=0;

		DWORD threadCount=sp.drives.size();//One for the copy thread

		if(threadCount == 0)
		{
			return 0;
		}

		DWORD threadIndex=0;
		int iThread=0;
		int i=0;
		INC_SYS_CALL_COUNT(GetSystemTime); // Needed to be INC TKH
		GetSystemTime(&acqTime);

		HANDLE *tHandle=NULL;
		fileSearch *search=NULL;
		DWORD exitCopyThread=1;

		tHandle=(HANDLE *) malloc(sizeof(HANDLE)*threadCount);
		search=(fileSearch *) malloc(sizeof(fileSearch)*threadCount);
		map<wstring,int >::iterator it;
		for(it=(sp.extensions).begin();it!=(sp.extensions).end();it++)
		{
			extensionList.push_back((*it).first);
		}

		maxFileSize=sp.maxFileSize;
		previousDays=sp.previousDays;

		
		tstring hashFileName = globalCaseDir + _T("\\Files\\Collected\\collected_files_incomplete.xml"); 
			

		AppLog.Write("Opening collected_files_incomplete.xml",WSLMSG_DEBUG);
		

		_tfopen_s(&fdHash,hashFileName.c_str(),_T("w"));

		if(fdHash != 0)
		{
			AppLog.Write("Opened the hash file for collection",WSLMSG_DEBUG);
		}
		else
		{
			AppLog.Write("Could not hash file for collection",WSLMSG_ERROR);
			return -1;
		}
		
		_ftprintf_s(fdHash,_T("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"));
		
		_ftprintf_s(fdHash,_T("<?xml-stylesheet type=\"text/xsl\" href=\"filedigest.xslt\"?>\n"));
		_ftprintf_s(fdHash,_T("<MD5Digest>\n"));



		
	
		memset(strbuf,0,1024);
		sprintf_s(strbuf,sizeof(strbuf),"In searchMultipleTargets");
		AppLog.Write(strbuf,WSLMSG_DEBUG);

		INC_SYS_CALL_COUNT(GetCurrentThread); // Needed to be INC TKH
		parentThreadId=GetCurrentThreadId();
		

	
		memset(strbuf,0,1024);
		sprintf_s(strbuf,sizeof(strbuf),"Main: Current Thread ID %d",parentThreadId);
		AppLog.Write(strbuf,WSLMSG_DEBUG);

		memset(strbuf,0,1024);
		sprintf_s(strbuf,sizeof(strbuf),"Main: Current State %d",w->currentState);
		AppLog.Write(strbuf,WSLMSG_DEBUG);
	

		rAgent=w;

		copyMutex=CreateMutex(NULL,FALSE,NULL);

		if(copyMutex==NULL)
		{

			memset(strbuf,0,1024);
			sprintf_s(strbuf,sizeof(strbuf),"Failed to create copyMutex");
			AppLog.Write(strbuf,WSLMSG_ERROR);
	
			_endthreadex(-1);

		}
		abortMutex=CreateMutex(NULL,FALSE,NULL);

		if(abortMutex==NULL)
		{
			memset(strbuf,0,1024);
			sprintf_s(strbuf,sizeof(strbuf),"Failed to create abortMutex");
			AppLog.Write(strbuf,WSLMSG_ERROR);
	
			_endthreadex(-1);

		}


		memset(tHandle,0,sizeof(HANDLE)*threadCount);
		memset(search,0,sizeof(fileSearch)*threadCount);
		i=0;
		gui.DisplayStatus(_T("Spawning search thread ..."));
		for(vector<wstring>::iterator dit=sp.drives.begin();dit<sp.drives.end();dit++)
		{
			search[i].drive=*dit;
	
			memset(strbuf,0,1024);
			sprintf_s(strbuf,sizeof(strbuf),"Main: Drive %s",to_utf8((*dit)).c_str());
			AppLog.Write(strbuf,WSLMSG_DEBUG);
				
			search[i].extension=L".*";
			search[i].createdDate=sp.createdDate;
			search[i].modifiedDate=sp.modifiedDate;
			tHandle[i]=(HANDLE)_beginthreadex(NULL,0,Thread,(void *)(&(search[i])),0,NULL);
			if(tHandle[i] == 0)
			{

				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"Main: Error creating thread %d %d",i,GetLastError());
				AppLog.Write(strbuf,WSLMSG_ERROR);
				
				_endthreadex(-1);
			}
			{
				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"Main:%d thread successful",i);
				AppLog.Write(strbuf,WSLMSG_DEBUG);

				
			}
			i++;
		}

		memset(strbuf,0,1024);
		sprintf_s(strbuf,sizeof(strbuf),"Main:Spawned %d threads",i);
		AppLog.Write(strbuf,WSLMSG_DEBUG);
		stcount=threadCount;

		//Copy thread
		HANDLE copyHandle=INVALID_HANDLE_VALUE;	
		unsigned int copyID=0;

		gui.DisplayStatus(_T("Spawning copy thread ..."));
		copyHandle=(HANDLE)_beginthreadex(NULL,0,copyThread,(void *)(&(sp.lattDrive)),0,&copyID);
		if(copyHandle == 0)
		{
			memset(strbuf,0,1024);
			sprintf_s(strbuf,sizeof(strbuf),"Error(%d) creating copy thread,freeing resources and exiting... ",GetLastError());
			AppLog.Write(strbuf,WSLMSG_ERROR);

			for(unsigned int i=0;i<threadCount;i++)
			{
				INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
				CloseHandle(tHandle[i]);
			}
			free(tHandle);
			_endthreadex(-1);
		}
		else
		{
			memset(strbuf,0,1024);
			sprintf_s(strbuf,sizeof(strbuf),"Spawned search thread ");
			AppLog.Write(strbuf,WSLMSG_DEBUG);
			
		}


		while(threadCount > 0)
		{
			threadIndex = WaitForMultipleObjects(threadCount,tHandle,FALSE,INFINITE);

			if(threadIndex == -1)
			{
				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"Main: Error(%d) Wait failed (in waiting for threads), freeing resources and exiting... ",GetLastError());
				AppLog.Write(strbuf,WSLMSG_ERROR);
			
				for(unsigned int i=0;i<threadCount;i++)
				{
					INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
					CloseHandle(tHandle[i]);
				}
				INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
				CloseHandle(copyHandle);
				free(search);
				free(tHandle);

				_endthreadex(-1);

			}
			iThread = (int) threadIndex - (int)WAIT_OBJECT_0;

			
			memset(strbuf,0,1024);
			sprintf_s(strbuf,sizeof(strbuf),"Main: Wait over for thread %d",iThread);
			AppLog.Write(strbuf,WSLMSG_DEBUG);

			GetExitCodeThread(tHandle[iThread],&exitCode);
			if(tHandle[iThread] != INVALID_HANDLE_VALUE)
			{
				INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
				if(!CloseHandle(tHandle[iThread]))
				{

					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"Main: Error(%d) CloseHandle",GetLastError());
					AppLog.Write(strbuf,WSLMSG_ERROR);
					_endthreadex(-1);
				}
				tHandle[iThread]=tHandle[threadCount - 1]; //Very Important
				threadCount--;
				stcount--;

				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"Main: iThread-%d exitCode-%d threadcode-%d threadIndex-%d",iThread,exitCode,threadCount,threadIndex);
				AppLog.Write(strbuf,WSLMSG_DEBUG);
			}
			else
			{

				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"Main :Error Invalid Handle -  exitCode %d errno %d",exitCode,GetLastError());
				AppLog.Write(strbuf,WSLMSG_ERROR);
				_endthreadex(-1);
			}

		}
		if(tHandle != NULL)
		{
			memset(strbuf,0,1024);
			sprintf_s(strbuf,sizeof(strbuf),"Main :Freeing memory for tHandle");
			AppLog.Write(strbuf,WSLMSG_DEBUG);
			free(tHandle);
			
		}
		if(search != NULL)
		{
			free(search);

			memset(strbuf,0,1024);
			sprintf_s(strbuf,sizeof(strbuf),"Main :Freeing memory for search");
			AppLog.Write(strbuf,WSLMSG_DEBUG);

		}

		for(;;)
		{
			INC_SYS_CALL_COUNT(WaitForSingleObject); // Needed to be INC TKH
			DWORD abortMutexRet=WaitForSingleObject(abortMutex,INFINITE);
			switch(abortMutexRet)
			{	
			case WAIT_OBJECT_0:
				if(Aborting())
				{

					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"Main :Aborting abort mutex");
					AppLog.Write(strbuf,WSLMSG_DEBUG);

					if(!ReleaseMutex(abortMutex))
					{

						memset(strbuf,0,1024);
						sprintf_s(strbuf,sizeof(strbuf),"Main :error releasing  abortMutex mutex");
						AppLog.Write(strbuf,WSLMSG_ERROR);
						_endthreadex(-1);
					}

				}
				else
				{
					
					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"Main :Releasing abort mutex");
					AppLog.Write(strbuf,WSLMSG_DEBUG);

					if(!ReleaseMutex(abortMutex))
					{

						memset(strbuf,0,1024);
						sprintf_s(strbuf,sizeof(strbuf),"Main :error releasing  abortMutex mutex");
						AppLog.Write(strbuf,WSLMSG_ERROR);

					}
				}
				break;
			case WAIT_ABANDONED:

				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"Main :error abortMutex abandoned %d",GetLastError());
				AppLog.Write(strbuf,WSLMSG_ERROR);
				break;
			case WAIT_FAILED:
				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"Main :error abortMutex failed %d",GetLastError());
				AppLog.Write(strbuf,WSLMSG_ERROR);
				break;
			}
			if(!threadCount && (filesToCopy.size()==0))
			{
				
				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"Main :killing copyThread");
				AppLog.Write(strbuf,WSLMSG_DEBUG);

				BOOL retCode;

				retCode=PostThreadMessage(copyID,WM_USER+1010,0L,0L);
				while(retCode != 0)
				{
					Sleep(2000);
					retCode=PostThreadMessage(copyID,WM_USER+1010,0L,0L);

					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"Main :PostThreadMessage to CopyThread Failed: %d [retry in 2s]",GetLastError());
					AppLog.Write(strbuf,WSLMSG_ERROR);
				}

			}
			INC_SYS_CALL_COUNT(WaitForSingleObject); // Needed to be INC TKH
			DWORD retVal;
			retVal=WaitForSingleObject(copyHandle,INFINITE);

			if(retVal==WAIT_OBJECT_0)
			{
				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"Main :thread count: %d ",threadCount);
				AppLog.Write(strbuf,WSLMSG_DEBUG);

				

				memset(strbuf,0,1024);
				sprintf_s(strbuf,sizeof(strbuf),"Main :files to copy: %d ",filesToCopy.size());
				AppLog.Write(strbuf,WSLMSG_DEBUG);

				if(abortFlag)
				{
					rAgent->currentState=ABORTING;
				}

				if(copyHandle != INVALID_HANDLE_VALUE)
				{
					INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
					CloseHandle(copyHandle);
				}
				
				if(abortMutex != INVALID_HANDLE_VALUE)
				{
					INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
					CloseHandle(abortMutex);

					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"Main :closed abortMutex");
					AppLog.Write(strbuf,WSLMSG_DEBUG);

				}
				if(copyMutex != INVALID_HANDLE_VALUE)
				{
					INC_SYS_CALL_COUNT(CloseHandle); // Needed to be INC TKH
					CloseHandle(copyMutex);

					
					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"Main :closed copyMutex");
					AppLog.Write(strbuf,WSLMSG_DEBUG);
				}

				if(fdHash)
				{

					
					memset(strbuf,0,1024);
					sprintf_s(strbuf,sizeof(strbuf),"Main :closing digest file after exiting from all threads");
					AppLog.Write(strbuf,WSLMSG_DEBUG);

					_ftprintf_s(fdHash,_T("</MD5Digest>\n"));


					CloseFileHandles(fdHash);
					
				}

				
				
				rAgent->currentState=CAPTURE_COMPLETE;
				
				break;
			}
		}
		return 0;

	}
	
}

#undef _UNICODE
#undef UNICODE