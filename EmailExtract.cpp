/**
** Author: Taylor K. Hanson
** Date Started: 06/01/2011
**
**TODO: fix the file name that is the destinatin path; actually link in and work the dll
**/

#include "EmailExtract.h"
#include <stdlib.h>
#include <crtdbg.h>

#pragma comment(linker, "/defaultlib:WSTLog.lib")

#ifdef _DEBUG
#define DEBUG_NEW new(_NORMAL_BLOCK, __FILE__, __LINE__)
#define new DEBUG_NEW
#endif

wofstream emailHash;
bool bmemWatermarkBreached = false;
   
/**
**This function does the RegOpenKey & the RegQueryKey in one -- since its used multiple times.
**It takes several IN parameters, but please note the OUT parameters, are they are used throughout the other functions.
**/
int RegOpenRegQueryKey(IN TCHAR* hkeyPath, OUT DWORD &email_subkeys_max_len, OUT HKEY &hkeyEmailProfiles, OUT DWORD &num_email_subkeys,OUT DWORD &num_of_values, OUT DWORD &email_profiles_values_name_len, OUT DWORD &email_profiles_values_len, WSTLog &AppLog)
{
	bool errorFlag = false;
	//Open Reg Key For Inspection
	if(RegOpenKeyEx(HKEY_CURRENT_USER, hkeyPath, 0, KEY_READ, &hkeyEmailProfiles) != ERROR_SUCCESS)
	{
		AppLog.Write("In Email Extract- Error Open Registry Key",WSLMSG_ERROR);
		//if(DEBUG)return ERR_REG_OPEN_KEY; // RETURN ERROR
        errorFlag = true;	
	}

	if(RegQueryInfoKey(hkeyEmailProfiles,
		NULL, NULL, NULL,
		&num_email_subkeys,
		&email_subkeys_max_len,
		NULL,
		&num_of_values,
		&email_profiles_values_name_len,
		&email_profiles_values_len,
		NULL,
		NULL
		) != ERROR_SUCCESS && 
		!errorFlag)
	{
		AppLog.Write("In Email Extract- Error Query Registry Key",WSLMSG_ERROR);
//		if(DEBUG)return ERR_REG_QUERY_INFO;	
		errorFlag = true;
	}
	if(errorFlag) return ERR_REG;
	return ERROR_SUCCESS;
}
/**
**In order to capsulate the code, I put the actual retrival of the .pst file location in a function.
**A few of the parameters actually come from RegOpenRegQuery function.
**/
int WindowsLiveMail(IN DWORD &email_subkeys_max_len, IN HKEY &hkeyEmailProfiles, IN DWORD &num_email_subkeys, IN DWORD &num_of_values, OUT DWORD &email_profiles_values_name_len, IN DWORD &email_profiles_values_len, IN TCHAR* pathForLiveMail, OUT LPBYTE &value, WSTLog &AppLog)
{
	int ret = 0;
	int found = -5;
	LPWSTR targetValue = TEXT("Store Root");

	//1. Check to see if there is any value that matches Store Root

			DWORD length_val_data=email_profiles_values_len + 1;

			//The values under the long windows live mail names are the length plus one for terminating character
			LPWSTR value_name_under_eprofile= new TCHAR[email_profiles_values_name_len + 1]; 
			ZeroMemory(value_name_under_eprofile, (email_profiles_values_name_len + 1));

			//Value is the size of the email profiles values length plus one for terminating charcter
			value= new BYTE[email_profiles_values_len + 1];
			ZeroMemory(value, (email_profiles_values_len + 1));
			DWORD length_val_eprofile = email_profiles_values_name_len + 1;

			for(int profile_vals=0; profile_vals < num_of_values; profile_vals++)
			{
				if((RegEnumValue(hkeyEmailProfiles, 
					profile_vals, 
					value_name_under_eprofile, 
					&length_val_eprofile,
					NULL, NULL, 
					value, 
					&length_val_data)) != ERROR_SUCCESS )
					{
						AppLog.Write("In Email Extract- Error Enumerating Registry Key",WSLMSG_ERROR);
						delete[] value;
						value = NULL;
						return ERR_REG_ENUM_KEY_SERIAL;
					}

				length_val_eprofile = email_profiles_values_name_len + 1;
				length_val_data = email_profiles_values_len + 1;

				if(_tcscmp(value_name_under_eprofile, targetValue) == 0)
				{
					delete[] value_name_under_eprofile;
					value_name_under_eprofile = NULL;
					return 0;
				}
				
			}
	delete[] value_name_under_eprofile;
	delete[] value;
	value_name_under_eprofile = NULL;
	value = NULL;
	return found;
}
int Hashing(TCHAR* FileToHash)
{
	//-->MD5 Hashing component

	FileHasher emailHasher;
	unsigned char md5hash[20];
	memset(md5hash,0,20);
	char p[3];

	//<--MD5 Hashing component
		memset(md5hash,0,20);
		emailHasher.MD5HashFile(FileToHash,md5hash);

		for (int i = 0; i < 16; ++i)
		{
			memset(p,0,3);
			sprintf(p,"%02X",md5hash[i]);
			emailHash<<p;
		}

		return 0;
}
int WriteToXML(TCHAR* OrignialFileName, TCHAR* CopiedFileName, bool CopyFlag)
{
	emailHash <<TEXT("<File type=\"email\">")                                                    <<endl;	
	emailHash <<TEXT("<CopiedFileName> ")   << CopiedFileName<< TEXT("</CopiedFileName>")        <<endl;
	emailHash <<TEXT("<OriginalFileName> ") << OrignialFileName<<TEXT( "</OriginalFileName>")    <<endl;
	emailHash <<TEXT("<Hash> ");
	Hashing(OrignialFileName);
	emailHash <<TEXT("</Hash>")                                                                  << endl;;

	struct __stat64 finfo;
	int result=_wstat64(OrignialFileName,&finfo);
	if(result == 0)
	{
		wstring owner;
		owner.clear();

		emailHash <<TEXT("<FileSize>")     << finfo.st_size <<" Bytes"    <<TEXT("</FileSize>")    <<endl;
		emailHash <<TEXT("<ModifiedTime>") << _ctime64(&(finfo.st_mtime)) <<TEXT("</ModifiedTime>")<<endl;
		emailHash <<TEXT("<CreatedTime>")  << _ctime64(&(finfo.st_ctime)) <<TEXT("</CreatedTime>") <<endl;;
		emailHash <<TEXT("<AccessTime>")   << _ctime64(&(finfo.st_atime)) <<TEXT("</AccessTime>")  <<endl;
		/*
		owner=FileSearch::GetOwnerInfo(OrignialFileName);
		GetAttr(OrignialFileName,recentHash);
		if(owner != L"")
		{
				emailHash<<"<Owner>"<<to_utf8(owner).c_str()<<"</Owner>"<<endl;
		}*/
	}


	if(CopyFlag)
	{
		emailHash <<TEXT("<CopyStatus>")<<TEXT("NOT COPIED")<<TEXT("</CopyStatus>")<<endl;
	}
	emailHash << TEXT("</File>") << endl;
	return 0;
}
/**
**In order to capsulate the code, I put the actual retrival of the .pst file location in a function.
**A few of the parameters actually come from RegOpenRegQuery function.
**/
int WindowsOutlook2010PSTPath( IN DWORD &email_subkeys_max_len, IN HKEY &hkeyEmailProfiles, IN DWORD &num_email_subkeys, IN DWORD &num_of_values, OUT DWORD &email_profiles_values_name_len, IN DWORD &email_profiles_values_len, IN TCHAR* pathForOutlook, OUT LPBYTE &value, IN LPWSTR targetValue, WSTLog &AppLog)
{
	int found = -5;

	// Save The Returned Handle as it will be overwritten later.
	HKEY hkeyOutlookEmailHandle = hkeyEmailProfiles;

	// Window Mail
	// Recieves The name of the value in a string with a terminating character. 
	TCHAR* email_profile = new TCHAR[email_subkeys_max_len+1];
	ZeroMemory(email_profile, email_subkeys_max_len+1);

	//Creating the extd path by determining the length required and memory allocation
	DWORD email_profile_key_length = _tcslen(pathForOutlook);
	email_profile_key_length += email_subkeys_max_len+1;
	TCHAR* extdPathForOutlook = new TCHAR[email_profile_key_length];
	ZeroMemory(extdPathForOutlook, email_profile_key_length);

	//Saving variables that will get overwritten for future use.
	DWORD num_keys_under_outlook = num_email_subkeys;
	DWORD email_profile_max_len=email_subkeys_max_len+1;


		int ret;
		DWORD lemail_profile_max_len=email_profile_max_len;
		// enumerate the subkeys
		if ((ret=RegEnumKeyEx(hkeyEmailProfiles,
			0,
			email_profile,
			&lemail_profile_max_len,
			NULL,
			NULL,
			NULL,
			NULL
			)) != ERROR_SUCCESS )
		{
			AppLog.Write("In Email Extract- Error Enumerating Registry Key",WSLMSG_ERROR);
			delete[] email_profile;
			delete[] extdPathForOutlook;
			email_profile = NULL;
			extdPathForOutlook = NULL;
			return ERR_REG_ENUM_KEY_SERIAL;
		}

	// for loop iteriating over the keys under outlook to look at their values.
	for(int i = 1; i < num_keys_under_outlook; i++)
	{	
		DWORD lemail_profile_max_len=email_profile_max_len;

		//1. Get the profile names to make the file path
		_tcscpy(extdPathForOutlook, pathForOutlook);
		_tcscat(extdPathForOutlook, email_profile);
	
		//2. Use the file path to check if there are values under it (Calling GetOutlook2007PSTFromMachine)
		
		int retFromGetOutlook=0;
		if ((retFromGetOutlook = RegOpenRegQueryKey(extdPathForOutlook,
			email_subkeys_max_len, 
			hkeyEmailProfiles,
			num_email_subkeys,
			num_of_values, 
			email_profiles_values_name_len, 
			email_profiles_values_len,
			AppLog)) != ERROR_SUCCESS)
		{
			//printf("Error in GetOutlook %d\n",retFromGetOutlook);
			delete[] email_profile;
			delete[] extdPathForOutlook;
			email_profile = NULL;
			extdPathForOutlook = NULL;
			return retFromGetOutlook;
		}
		//3. Check if there are any values under the email_profile selected above
		if(num_of_values != 0)
		{
			//4. Check to see if there is any value that matches 001f6700
			DWORD eprofile_val_type= 3;
			DWORD length_val_data=64;

			//The values under the long outlook names are the length plus one for terminating character
			LPWSTR value_name_under_eprofile = new TCHAR[email_profiles_values_name_len + 1]; 
			ZeroMemory(value_name_under_eprofile, (email_profiles_values_name_len + 1));

			//Value is the size of the email profiles values length plus one for terminating charcter
		    value= new BYTE[email_profiles_values_len + 1];
			ZeroMemory(value, (email_profiles_values_len + 1));
			DWORD length_val_eprofile = email_profiles_values_name_len + 1;

			for(int profile_vals=0; profile_vals < num_of_values; profile_vals++)
			{
				if((RegEnumValue(hkeyEmailProfiles, 
					profile_vals, 
					value_name_under_eprofile, 
					&length_val_eprofile,
					NULL, NULL, 
					value, 
					&length_val_data)) != ERROR_SUCCESS )
					{
						AppLog.Write("In Email Extract- Error Enumerating Registry Key",WSLMSG_ERROR);
						delete[] email_profile;
						delete[] extdPathForOutlook;
						delete[] value_name_under_eprofile;
						email_profile = NULL;
						extdPathForOutlook = NULL;
						value_name_under_eprofile = NULL;
						return ERR_REG_ENUM_KEY_SERIAL;
					}

				length_val_eprofile = email_profiles_values_name_len + 1;
				length_val_data = email_profiles_values_len + 1;

				if(_tcscmp(value_name_under_eprofile, targetValue) == 0)
				{
					AppLog.Write("Found Windows OutLook PST File",WSLMSG_STATUS);
					delete[] email_profile;
					delete[] extdPathForOutlook;
					delete[] value_name_under_eprofile;
					email_profile = NULL;
					extdPathForOutlook = NULL;
					value_name_under_eprofile = NULL;
					return 0;
				}
				
			}
			delete[] value;
			delete[] value_name_under_eprofile;
			value = NULL;
			value_name_under_eprofile=NULL;
		}
		//5. Get next email_profile and repeat above; So use original handle
		if ((ret=RegEnumKeyEx(hkeyOutlookEmailHandle,
			i,
			email_profile,
			&lemail_profile_max_len,
			NULL,
			NULL,
			NULL,
			NULL)) != ERROR_SUCCESS )
		{
			AppLog.Write("In Email Extract- Error Enumerating Registry Key",WSLMSG_ERROR);
			delete[] email_profile;
			delete[] extdPathForOutlook;
			email_profile = NULL;
			extdPathForOutlook = NULL;
			return ERR_REG_ENUM_KEY_SERIAL;
		}
		else
		{
			RegOpenRegQueryKey(
				extdPathForOutlook, 
				email_subkeys_max_len, 
				hkeyEmailProfiles, 
				num_email_subkeys, 
				num_of_values, 
				email_profiles_values_name_len, 
				email_profiles_values_len,
				AppLog);
		}

	}

	delete[] email_profile;
	delete[] extdPathForOutlook;
	email_profile = NULL;
	extdPathForOutlook = NULL;
	return found;
}

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
		GetDiskFreeSpaceEx(NULL,(PULARGE_INTEGER)&availSpace,NULL,NULL);

		if((availSpace - TotalFileSize.QuadPart) <= MINIMUM_SPACE_REQUIRED)
		{
			*((BOOL *)(lpData))=TRUE;
			return PROGRESS_STOP;
		}
		return PROGRESS_CONTINUE;
}
int CopyEmailFiles(IN TCHAR* fileToCopy, IN TCHAR* fileExt, TCHAR* file, WSTLog &AppLog)
{
	//Added this buffer because once it zero memory is called, wcslen(path) returns 0; so I store the actual length in a buffer before hand
	int buffLen = (_tcslen(TEXT("Files\\MailFiles\\")) + _tcslen(file) + _tcslen(fileExt) + 1);
	TCHAR* path = new TCHAR[buffLen];
	ZeroMemory(path, buffLen);

	_tcscpy_s(path,buffLen,TEXT("Files\\MailFiles\\"));
	_tcscat_s(path,buffLen, file);
	_tcscat_s(path,buffLen, fileExt);

	if (!bmemWatermarkBreached)
	{
		if(!CopyFileEx(
			fileToCopy,
			path,
			CopyProgressRoutine,
			LPVOID(&bmemWatermarkBreached),
			FALSE,
			COPY_FILE_RESTARTABLE))
		{
			AppLog.Write("In Email Extract- CopyFileEx failed, Attempting Shadow Copy",WSLMSG_STATUS);
			STARTUPINFOA si;
			PROCESS_INFORMATION pi;
			boolean bResult = FALSE;

			ZeroMemory(&pi, sizeof(pi));
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(STARTUPINFOA);
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_SHOW;

			BOOL f64 = FALSE;
			char cmdLine[MAX_PATH *3];
			memset(cmdLine,0,MAX_PATH+1);

			char mFileToCopy[MAX_PATH + 1]	;
			char mPath[MAX_PATH + 1]	;
			ZeroMemory(mFileToCopy, MAX_PATH);
			ZeroMemory(mPath, MAX_PATH);

			wcstombs(mFileToCopy, fileToCopy, MAX_PATH);
			wcstombs(mPath, path, MAX_PATH);

			IsWow64Process(GetCurrentProcess(), &f64);
			if(f64 )
			{
				sprintf(cmdLine,"%s %s %s", "WSTLockedFileCopy_x64.exe", mFileToCopy, mPath);
				//			dcout(cmdLine);
				bResult = CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
			}
			else
			{
				sprintf(cmdLine,"%s %s %s", "WSTLockedFileCopy.exe", mFileToCopy, mPath);
				//dcout(cmdLine);
				bResult = CreateProcessA(NULL, cmdLine, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi);
			}
			if (bResult)
			{
				WaitForSingleObject(pi.hThread, INFINITE);

				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
			else
			{
				AppLog.Write("In Email Extract- Shadow Copy Failure",WSLMSG_ERROR);
				delete[] path;
				return ERR_SHW_CPY;
			}

		}
	}
	else
	{
		//memset(strbuf,0,1024);
		//sprintf_s(strbuf,sizeof(strbuf),"Metadata information for %s collected [File not copied due to insufficient space on the device]", OriginalFileName);
		//AppLog.Write(strbuf,WSLMSG_STATUS);
		return CPY_INSUFF_SPACE_ERR;
	}
	delete[] path;
	return 0;
}
int FormatDirectoryToTraverse(TCHAR* directoryToTraverse, TCHAR* addition)
{
		DWORD findFirstPathLength = _tcslen(directoryToTraverse)+1;
		findFirstPathLength += _tcslen(addition);
		
		TCHAR* properDirectoryToTraverse = new TCHAR[findFirstPathLength];
		ZeroMemory(properDirectoryToTraverse, findFirstPathLength);

		_tcscpy_s(properDirectoryToTraverse, findFirstPathLength, directoryToTraverse);
		_tcscat_s(properDirectoryToTraverse, findFirstPathLength, addition);
	
		memset(directoryToTraverse, 0, MAX_PATH);

		_tcscpy_s(directoryToTraverse, MAX_PATH, properDirectoryToTraverse);
		delete [] properDirectoryToTraverse;
		properDirectoryToTraverse = NULL;

		return 0;
}
int RemoveEnvironmentVariable(TCHAR* directoryToTraverse, WSTLog &AppLog)
{
		TCHAR* pos = NULL;
	// Format the size and type of environment variable
		TCHAR* path = new TCHAR[MAX_PATH];
		TCHAR* environmentVariable = new TCHAR[MAX_PATH];
		ZeroMemory(environmentVariable, MAX_PATH);

	// The remaining path minus the environment variable
		TCHAR* remainingPath = new TCHAR[_tcslen(directoryToTraverse)];
		ZeroMemory(remainingPath, _tcslen(directoryToTraverse));

	// Strtok the path to pluck out the environment variable
		_tcscpy(environmentVariable, _tcstok_s(directoryToTraverse, TEXT("%"), &pos));
		_tcscpy(remainingPath, _tcstok_s(NULL, TEXT("%"), &pos));
		if(remainingPath == NULL)
		{
			AppLog.Write("Path in EnvironmentVariable is NULL",WSLMSG_DEBUG);
			return ERR_ENVAR;
		}
	 // append and reformat the path
		GetEnvironmentVariable(environmentVariable, path, (MAX_PATH));
		_tcsncat(path, remainingPath, MAX_PATH);

		/*delete[] directoryToTraverse;
		directoryToTraverse = NULL;
		directoryToTraverse = new TCHAR[_tcslen(path)+1];*/
		memset(directoryToTraverse, 0, MAX_PATH + 1);
		_tcscpy_s(directoryToTraverse, MAX_PATH + 1, path);

	delete[] environmentVariable;
	delete[] remainingPath;	
	delete[] path;
	environmentVariable = NULL;
	remainingPath = NULL;
	path = NULL;
	return SUCCESS;
}
int GetNewFileName(TCHAR* fileName)
{
	static int count = 1;
	TCHAR* a = new TCHAR[5];   
	ZeroMemory(a, 5);

	_tcscpy(fileName, MF);
	_tcscat(fileName, _itow(count, a, BASE));

	count ++;
	delete[] a;
	return 0;
}
int RecursivelyTraverseDirectory(TCHAR* directoryToTraverse, TCHAR* fileExtToLookFor, WSTLog &AppLog)
{
	WIN32_FIND_DATA FileData;
	HANDLE hSearch;
	TCHAR* asterisk = TEXT("*");
	TCHAR* slash = TEXT("\\");
	TCHAR* newPathToTraverse = NULL;

	//Also environment variables are a NO NO , so make them not so.
	if(_tcsncmp(&(directoryToTraverse[0]), TEXT("%"), 1) == 0)
	{		
		RemoveEnvironmentVariable(directoryToTraverse, AppLog);
	}
	
	//FindFirstFile does NOT like strings that end in "\", its not okay, it needs a *.
	if(_tcscmp(&directoryToTraverse[wcslen(directoryToTraverse) - 1], TEXT("\\")) == 0)
	{
		FormatDirectoryToTraverse(directoryToTraverse, asterisk);
	}

	if((hSearch = FindFirstFile(directoryToTraverse, &FileData)) != INVALID_HANDLE_VALUE)
	{
			// add the .eml to the end of the directoryToTraverse
			FormatDirectoryToTraverse(directoryToTraverse, fileExtToLookFor);
		do{
			if((FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (_tcscmp(FileData.cFileName, TEXT("..")) != 0) && (_tcscmp(FileData.cFileName, TEXT(".")) != 0))
			{
				//compares to see if the last 4 [. e m l || . p s t] characters in the path match up with the file ext.
				if(_tcscmp(&directoryToTraverse[_tcslen(directoryToTraverse) - 4], fileExtToLookFor) == 0)
				{
					// gets rid of the last 5 [* . e m l || * . p s t] characters and terminates the new string
					directoryToTraverse[_tcslen(directoryToTraverse) - 5] = 0;
				}
				newPathToTraverse = new TCHAR[MAX_PATH + 1];
				ZeroMemory(newPathToTraverse , MAX_PATH + 1);
				_tcscpy(newPathToTraverse , directoryToTraverse);
				_tcscat(newPathToTraverse,FileData.cFileName);
				FormatDirectoryToTraverse(newPathToTraverse, slash);
			
				RecursivelyTraverseDirectory(newPathToTraverse, fileExtToLookFor, AppLog);
				delete[] newPathToTraverse;
			}
			// similar to above, only this time its for copying.
			if(_tcscmp(&FileData.cFileName[_tcslen(FileData.cFileName) - 4], fileExtToLookFor) == 0)
			{   
				TCHAR* newPathToCopy = new TCHAR[wcslen(directoryToTraverse) + wcslen(FileData.cFileName) + 1];
				ZeroMemory(newPathToCopy,_tcslen(directoryToTraverse) + _tcslen(FileData.cFileName) + 1);
				_tcscpy_s(newPathToCopy,_tcslen(directoryToTraverse) + _tcslen(FileData.cFileName) + 1, directoryToTraverse);

				newPathToCopy[_tcslen(directoryToTraverse) - 5] = 0;
				_tcscat(newPathToCopy,FileData.cFileName);
				
				TCHAR* fileName = new TCHAR[MFL];
				GetNewFileName(fileName);
				int i = CopyEmailFiles(newPathToCopy, fileExtToLookFor, fileName, AppLog);
				if(!CPY_INSUFF_SPACE_ERR)
				{
					WriteToXML(newPathToCopy, fileName, i);
				}
				delete[] fileName;
				delete[] newPathToCopy;
				newPathToCopy = NULL;
				fileName = NULL;

				if(CPY_INSUFF_SPACE_ERR)
				{
					FindClose(hSearch);
					return CPY_INSUFF_SPACE_ERR;
				}
			}
		}while(FindNextFile(hSearch, &FileData));
	}
	else
	{
		FindClose(hSearch);
		return ERR_FIND_FIRST;
	}
}
/*int ParseProfileIni(TCHAR* pathProfileIni)
{
	RemoveEnvironmentVariable(pathProfileIni);
	FILE* FileProfileIni;
	FileProfileIni = _wfopen(pathProfileIni, TEXT("r"));
	if(FileProfileIni == NULL)
	{
		return ERR_FILE_NOT_EXIST;
	}
	dcout("successssss");
	fclose(FileProfileIni);
	return 0;
}*/
//The next step is to create this function.
MAILFILECAPTURE_EXPORT int MailFileCapture(WSTLog &AppLog)
{ 
	AppLog.Write("In Email Extract",WSLMSG_STATUS);

	static TCHAR directoryToTraverse[MAX_PATH+1];
	ZeroMemory(directoryToTraverse, MAX_PATH+1);

	bool errorFlag = false;
	int errorCode = SUCCESS;
	HKEY hkeyEmailProfiles = NULL;
	DWORD email_subkeys_max_len = 0;
	DWORD num_email_subkeys = 0;
	DWORD num_of_values = 0;
	DWORD email_profiles_values_name_len = 0;
	DWORD email_profiles_values_len = 0;
	LPBYTE value;

	LPWSTR targetValue2007 = TEXT("001f6700");

	TCHAR g_tempBuf[MAX_PATH + 1];
	GetCurrentDirectory(MAX_PATH,g_tempBuf);
	tstring emailFile = g_tempBuf;
	emailFile += _T("\\Files\\MailFiles\\MailFiles_incomplete.xml");
	emailHash.open(emailFile.c_str());
	emailHash<<"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";	
	emailHash<<"<?xml-stylesheet type=\"text/xsl\" href=\"filedigest.xslt\"?>\n";
	emailHash<<"<MD5Digest>"<<endl;

	//Initially call and open the key.
	if(RegOpenRegQueryKey(
		REGPATHOUTLOOK, 
		email_subkeys_max_len,
		hkeyEmailProfiles, 
		num_email_subkeys, 
		num_of_values, 
		email_profiles_values_name_len, 
		email_profiles_values_len,
		AppLog) != ERROR_SUCCESS)
	{
		AppLog.Write("Could not successfully find and open the Windows Outlook Registry Key",WSLMSG_STATUS);
		errorFlag = true;
	}

	//Windows Outlook 2010 PST Path Function
	if(int ret = WindowsOutlook2010PSTPath(
		email_subkeys_max_len, 
		hkeyEmailProfiles,
		num_email_subkeys, 
		num_of_values, 
		email_profiles_values_name_len, 
		email_profiles_values_len, 
		REGPATHOUTLOOK,
		value,
		targetValue2007,
		AppLog) != 0
		&& !errorFlag)
	{
		AppLog.Write("Did not Find Windows Outlook 2003/2007/2010 PST File",WSLMSG_ERROR);
		errorFlag = true;
	}


	if(!errorFlag)
	{
		TCHAR* fileName = new TCHAR[MFL];
		GetNewFileName(fileName);

		errorCode = CopyEmailFiles((TCHAR*)value, TEXT(".pst"), fileName, AppLog);
		if(errorCode >= 0)
		{
			WriteToXML((TCHAR*)value, fileName, true);
		}

		delete[] fileName;
		delete[] value;
		fileName = NULL;
		value = NULL;
	}

	if(!errorCode == CPY_INSUFF_SPACE_ERR)
	{
		//reset error flag for .eml files
		errorFlag = false;

		if(RegOpenRegQueryKey(
			REGPATHLIVEMAIL, 
			email_subkeys_max_len, 
			hkeyEmailProfiles, 
			num_email_subkeys, 
			num_of_values, 
			email_profiles_values_name_len, 
			email_profiles_values_len,
			AppLog))
		{
			AppLog.Write("Could not successfully find and open the Windows Live Mail Registry Key",WSLMSG_STATUS);
			errorFlag = true;
		}

		if( int ret = WindowsLiveMail(
			email_subkeys_max_len, 
			hkeyEmailProfiles,
			num_email_subkeys, 
			num_of_values, 
			email_profiles_values_name_len, 
			email_profiles_values_len, 
			REGPATHLIVEMAIL,
			value,
			AppLog) != 0
			&& !errorFlag)
		{
			AppLog.Write("Did not find any Windows Live Mail Location",WSLMSG_ERROR);
			errorFlag = true;
		}
		if(!errorFlag)
		{
			_tcscpy(directoryToTraverse, (TCHAR*)value);
			errorCode = RecursivelyTraverseDirectory(directoryToTraverse, TEXT(".eml"), AppLog);
			delete[] value;
			value = NULL;
			if(errorCode != CPY_INSUFF_SPACE_ERR) errorCode = SUCCESS;
		}
	}
	//close xml file
	emailHash<<"</MD5Digest>"<<endl;
	emailHash.close();
	//_CrtDumpMemoryLeaks();

	return errorCode;
}