#define _UNICODE
#define UNICODE

#include "stdafx.h"
#include "extractreg.h"
#include "HashedXML.h"
#include "wstlog.h"
#include "WSTRemote.h"
#include "Shlobj.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

//#define GB2 2147483640
#define GB4 4294967290

extern string to_utf8(const wstring &);
extern int RestoreAccessTime(TCHAR *fileName, FILETIME atime);//cfilefind.cpp
extern int GetFileAccessTime(TCHAR *fileName,FILETIME &accessTime);//cfilefind.cpp

vector<LPTSTR> HKCUSubKeyNames;
vector<LPTSTR> HKLMSubKeyNames;
vector<LPTSTR> XMLNodeName;
vector<LPTSTR> HKLMXMLNodeName;
vector<LPTSTR> XMLSubNodeName;
vector<LPTSTR> HKLMXMLSubNodeName;
map<const TCHAR *,HKEY> NameKeyMap;

ofstream mruHash;
extern string os_name;

#ifdef _UNICODE
extern tstring XMLDelimitString(tstring szSource);
extern tstring XMLDelimitString(const TCHAR *szValue);
#endif

extern string XMLDelimitString(string szSource);
extern string XMLDelimitString(const char *szValue);

extern bool isImage(string extension);
extern char *imageList[7];
extern wstring escapeXML(wstring str);

//Copy constraint functions
namespace FileSearch
{
	//2. File size
	extern bool isWithinFileSize(unsigned long long fileSize);
	//time check
	extern BOOL dateCheck(FILETIME ftime);
	//3. File time (Created and/or Last written)
	extern bool isWithinLastPreviousDays(FILETIME ftime,FILETIME ctime,int createdDate,int modifiedDate);
}


namespace Registry
{
//1. File type
	vector<wstring> extensions;

	bool inExtensionList(wstring filename)
	{
		for(vector<wstring>::iterator it=Registry::extensions.begin();it!=Registry::extensions.end();it++)
		{
			wstring ext((*it).begin(),(*it).end());
			if(!(_tcsicmp(ext.c_str(),(filename.substr(filename.find_last_of(L'.')+1)).c_str())))
			{
				return true;
			}
//			return !(extensionList.find(filename.substr(filename.find_last_of(L'.')+1)) == extensionList.end());
		}
		return false;
	}

	unsigned long long maxFileSize;

	bool isWithinFileSize(unsigned long long fileSize)
	{
		if(fileSize < 0)
			return -2;	// Invalid file size
		else if(fileSize > GB4)
			return -1;	// Larger than FAT32 4GB limit 
		else if(fileSize > Registry::maxFileSize)
			return 0;	// Larger than user specified limit - ignore
		else
			return 1;	// ok
	}

}

void WriteNodeToXML(HashedXMLFile &repository,LPTSTR NodeName,int open) //1-Open 0-Close
{
	tstring tNodeName = NodeName; 
	if(open)
	{
		repository.OpenNode(const_cast<char *> (to_utf8(tNodeName).c_str()));
	}
	else
	{
		repository.CloseNode(const_cast<char *> (to_utf8(tNodeName).c_str()));
	}
}

 string GetExtension(tstring filePath)
 {

	 tstring extW= filePath.substr(filePath.find_last_of(L'.')+1);
	 string extensionA(extW.begin(),extW.end());
	 return extensionA;
 }

tstring GetNextFileName(int index,string ext,string prefix,string dstDirectory,string &baseName)
{
	ostringstream dstFileName;

	ostringstream ss;
	string s="00000";
	ss.str("");

	if(index<=(pow(2.0,16.0)-5))
	{
		ss<<s.substr(0,s.length()-((((int)(log((double)(index))/log(10.0)))+1)))<<index;
	}
	dstFileName.str("");
	baseName.clear();
	baseName="File"+ss.str()+"."+ext;
	dstFileName<<dstDirectory.c_str()<<"\\"<<prefix.c_str()<<ss.str().c_str()<<"."<<ext;

	string dstFileNameA = dstFileName.str();
	tstring dstFileNameT(dstFileNameA.begin(),dstFileNameA.end());

	return dstFileNameT;
}
namespace FileSearch
{
	extern wstring GetOwnerInfo(wstring);
}


	

DWORD GetAttr(wstring fileName,ofstream &mruHash)
{
	//Retrieve the file attributes

	DWORD AttrList[]={FILE_ATTRIBUTE_READONLY,FILE_ATTRIBUTE_HIDDEN,FILE_ATTRIBUTE_ARCHIVE};
	char *AttrValue[]={"READONLY","HIDDEN","ARCHIVE"};

	DWORD fileattr=0;
	fileattr=GetFileAttributes(fileName.c_str());

	for(int i=0;i<(sizeof(AttrList)/sizeof(DWORD));i++)
	{
		mruHash<<"<"<<AttrValue[i]<<">"<<(((fileattr&AttrList[i])!=0)?"TRUE":"FALSE")<<"</"<<AttrValue[i]<<">"<<endl;
	}
	return 0;
}

namespace FileSearch
{
	extern bool bmemWatermarkBreached;
	extern DWORD CALLBACK CopyProgressRoutine(
		__in      LARGE_INTEGER TotalFileSize,
		__in      LARGE_INTEGER TotalBytesTransferred,
		__in      LARGE_INTEGER StreamSize,
		__in      LARGE_INTEGER StreamBytesTransferred,
		__in      DWORD dwStreamNumber,
		__in      DWORD dwCallbackReason,
		__in      HANDLE hSourceFile,
		__in      HANDLE hDestinationFile,
		__in_opt  LPVOID lpData
		);
}

int CopyOpenSaveMRUFiles(tstring filePath,FILETIME accessTime)
{
	size_t found;
	tstring dstPath;
	char *mruPath;
	tstring filename;
	FileHasher mruhasher;
	char p[3];

	static int fileCount= 0;

	string ext= GetExtension(filePath);
	string baseName;
	tstring dstFileName=GetNextFileName(fileCount,ext,"File","Files\\MRU",baseName);
	fileCount++;

	int copyStatus=0;


	mruPath=(char *)dstFileName.c_str();

	//To restore access times
	TCHAR accTime[100]; 

	SYSTEMTIME saccessTime;
	FileTimeToSystemTime(&accessTime,&saccessTime);
	StringCchPrintf(accTime, 100, TEXT("%02d/%02d/%d  %02d:%02d"),saccessTime.wMonth, saccessTime.wDay, saccessTime.wYear,saccessTime.wHour, saccessTime.wMinute);
	//To restore access times

	//IFCAP_CHANGES follow
	char msgBuf[2*MAX_PATH];
	memset(msgBuf,0,2*MAX_PATH);
	copyStatus=0;
	if(!FileSearch::bmemWatermarkBreached)
	{
		gui.DisplayResult(_T("Registry: Copying File ..."));
		memset(gui.largeUpdateMsg,0,sizeof(TCHAR) *(LARGE_MSG_SIZE));
		_stprintf(gui.largeUpdateMsg,_TEXT("%s %s"),TEXT("Copying File: "),(filePath.c_str()));
		gui.DisplayStatus(gui.largeUpdateMsg);

		if(!CopyFileEx((LPCTSTR)filePath.c_str(),(LPCTSTR)(mruPath),FileSearch::CopyProgressRoutine,LPVOID(&FileSearch::bmemWatermarkBreached),FALSE,COPY_FILE_RESTARTABLE))
		{
			if(GetLastError() == ERROR_REQUEST_ABORTED)//IFCAP_CHANGES
			{	
				sprintf_s(msgBuf,sizeof(msgBuf),"Metadata information for %s collected [File not copied due to insufficient space on the device]",to_utf8(filePath).c_str());
				gui.DisplayWarning(_T("Could not copy file due to insufficient space"));
				AppLog.Write(msgBuf,WSLMSG_STATUS);
			}
			else
			{
				gui.DisplayError(_T("Could not copy file."));
				sprintf_s(msgBuf,sizeof(msgBuf),"Error Could not copy File %s Error %d",to_utf8(filePath).c_str(),GetLastError());
				AppLog.Write(msgBuf,WSLMSG_ERROR);
			}

		}
		else
		{
			copyStatus = 1;
		}
	}

	//if(CopyFile((LPCTSTR)filePath.c_str(),(LPCTSTR)(mruPath),FALSE))
	if(FileSearch::bmemWatermarkBreached || copyStatus == 1)
	{
		//SetLastError(0);


		//Compute hash

		filename=filePath;
		unsigned char md5hash[20];
		memset(md5hash,0,20);
		mruhasher.MD5HashFile(filename.c_str(),md5hash);

		if(isImage(ext))
		{
			mruHash<<"<File type=\"image\">"<<endl;		
		}
		else
		{
			mruHash<<"<File>"<<endl;		
		}
		//mruHash<<"<CopiedFileName> "<<(to_utf8(XMLDelimitString(dstFileName).c_str())).c_str()<<"</CopiedFileName>"<<endl;
		mruHash<<"<CopiedFileName> "<<XMLDelimitString(baseName).c_str()<<"</CopiedFileName>"<<endl;
		mruHash<<"<OriginalFileName> "<<(to_utf8(XMLDelimitString(filename).c_str())).c_str()<<"</OriginalFileName>"<<endl;
		mruHash<<"<Hash> ";
/*
		gui.DisplayState(_T("Copying File MRUList"));
		memset(gui.largeUpdateMsg,0,sizeof(TCHAR) *(LARGE_MSG_SIZE));
		_stprintf(gui.largeUpdateMsg,_TEXT("%s %s"),TEXT("Copying File: "),(filename.c_str()));
		gui.DisplayStatus(gui.largeUpdateMsg);
*/
		for (int i = 0; i < 16; ++i)
		{
			memset(p,0,3);
			sprintf(p,"%02X",md5hash[i]);
			mruHash<<p;
			memset(p,0,3);
		}
		mruHash<<"</Hash>"<<endl;

		struct __stat64 finfo;
		int result=_wstat64(filePath.c_str(),&finfo);
		if(result == 0)
		{
			wstring owner;
			owner.clear();

			mruHash<<"<FileSize>"<<finfo.st_size<<" Bytes"<<"</FileSize>"<<endl;
			mruHash<<"<ModifiedTime>"<<_ctime64(&(finfo.st_mtime))<<"</ModifiedTime>"<<endl;
			mruHash<<"<CreatedTime>"<<_ctime64(&(finfo.st_ctime))<<"</CreatedTime>"<<endl;;
			mruHash<<"<AccessTime>"<<to_utf8(accTime)<<"</AccessTime>"<<endl;

			owner=FileSearch::GetOwnerInfo(filePath);
			GetAttr(filePath,mruHash);
			if(owner != L"")
			{
				mruHash<<"<Owner>"<<to_utf8(owner).c_str()<<"</Owner>"<<endl;
			}
		}
		if(FileSearch::bmemWatermarkBreached)
		{
			memset(msgBuf,0,2*MAX_PATH);
			sprintf_s(msgBuf,sizeof(msgBuf),"Metadata information for %s collected [File not copied due to insufficient space on the device]",to_utf8(filePath).c_str());
			AppLog.Write(msgBuf,WSLMSG_STATUS);
			mruHash<<"<CopyStatus>"<<"NOT COPIED"<<"</CopyStatus>"<<endl;
		}
		mruHash<<"</File>"<<endl;
	}
	return 0;
}



void WriteValueToXML(LPBYTE value,DWORD valueType,DWORD valueLen,HashedXMLFile &repository,LPTSTR subNodeName,LPCTSTR displayName,ScanConfig const &config)
{
	tstring tdisplayName = displayName;
	repository.OpenNode(const_cast<char *>( to_utf8(tdisplayName).c_str() ));
	tstring tsubNodeName = subNodeName; 
	repository.OpenNode(const_cast<char *> (to_utf8(tsubNodeName).c_str()) );
	
	tstring filePath;
	
	tstring escapedVal;

	FILETIME accessTime;//To restore access times.
	memset(&accessTime,0,sizeof(FILETIME));
	wchar_t FileSizeMessage[MAX_PATH+1+100];
	memset(FileSizeMessage,0,(MAX_PATH+1+100)*sizeof(wchar_t));

	switch(valueType)
	{
		case REG_SZ:
			escapedVal=XMLDelimitString(const_cast<TCHAR *>((LPTSTR)value));
			repository.Write((to_utf8(escapedVal)).c_str());
			if(subNodeName==_T("File"))
			{

				GetFileAccessTime(const_cast<TCHAR *>(filePath.c_str()),accessTime);
				filePath=(TCHAR* )value;//XP only
				struct __stat64 finfo;
				int result=_wstat64(filePath.c_str(),&finfo);

				if(result == 0)
				{
					int retValidSize = Registry::isWithinFileSize(finfo.st_size);

					if(Registry::inExtensionList(filePath) && retValidSize > 0)
					{
						CopyOpenSaveMRUFiles(filePath,accessTime);
					}
					else
					{
					
						if(retValidSize < 1) // Not within File Size
						{
							wcscpy_s(FileSizeMessage,MAX_PATH+100, filePath.c_str());
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
						}
					}
					RestoreAccessTime(const_cast<TCHAR *>(filePath.c_str()),accessTime);
				}
			}
			break;
		case REG_BINARY:
		
//#ifdef testreg
			LPBYTE pV = NULL;
			pV=value;		
			LPBYTE IDList = NULL;
			TCHAR tbuf[MAX_PATH+1];
			if(subNodeName == _T("Cmd"))
			{

				wstring wbuf((wchar_t *)pV);
				if(wbuf.size() != 1)
				{
					string ustr= to_utf8(XMLDelimitString(wbuf)).c_str();
					repository.Write(ustr.c_str());
				}
				IDList = (pV+(wbuf.length()+1)*sizeof(TCHAR));

				wstring fileName((wchar_t *)IDList);

				if(os_name == "XP")//IDList is the file name
				{
					if(fileName.size() != 1)
					{
						string ustr= to_utf8(XMLDelimitString(fileName)).c_str();
						repository.Write(" last used in directory ");
						repository.Write(ustr.c_str());
					}
				}
				else if((os_name == "VISTA") ||(os_name == "WIN7"))//IDList is a ItemIdList
				{
					memset(tbuf,0,sizeof(tbuf));		
					if(SHGetPathFromIDList((LPCITEMIDLIST)IDList,tbuf))
					{
						repository.Write(" last used in directory ");
						
						repository.Write(to_utf8(XMLDelimitString(tbuf)).c_str());
					}
				}
			}
			else if(subNodeName == _T("File"))
			{
				memset(tbuf,0,sizeof(tbuf));
				SHGetPathFromIDList((LPCITEMIDLIST)pV,tbuf);

				GetFileAccessTime(tbuf,accessTime);
				struct __stat64 finfo;
				int result=_wstat64(tbuf,&finfo);

				if(result == 0)
				{
					int retValidSize = Registry::isWithinFileSize(finfo.st_size);

					if(Registry::inExtensionList(tbuf) && retValidSize > 0)
					{
						CopyOpenSaveMRUFiles(tbuf,accessTime);
						repository.Write(to_utf8(escapeXML(tbuf).c_str()).c_str());
					}
					else
					{
						if(retValidSize < 1) // Not within File Size
						{
							wcscpy_s(FileSizeMessage,MAX_PATH+100, tbuf);
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
						}
					}
					RestoreAccessTime(tbuf,accessTime);
				} // end of result 
	
			}
			else
			{
				wstring wbuf((wchar_t *)pV);
				if(wbuf.size() != 1)
				{
					string ustr= to_utf8(XMLDelimitString(wbuf)).c_str();
					repository.Write(ustr.c_str());
				}
			}
			break;
//#endif
	}
	repository.CloseNode(const_cast<char *> (to_utf8(tsubNodeName).c_str()));
}



void InitializeHKCURegistryMaps(ScanConfig const &config) //const since you do't want to change and reference to save memory
{
	TCHAR g_tempBuf[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,g_tempBuf);
	tstring mruFile = g_tempBuf;
	mruFile += _T("\\Files\\MRU\\mrudigest_incomplete.xml");
	mruHash.open(mruFile.c_str());
	mruHash<<"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";	
	mruHash<<"<?xml-stylesheet type=\"text/xsl\" href=\"filedigest.xslt\"?>\n";
	mruHash<<"<MD5Digest>"<<endl;
	
	if((g_osvex.dwMajorVersion == 6) &&(g_osvex.dwMinorVersion==1)) // Windows 7 has a different location
	{
		if(config.getRecentDocs)
		{
			HKCUSubKeyNames.push_back(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSavePidlMRU\\*"));
		}
		if(config.getRecentlyRun)
		{
			HKCUSubKeyNames.push_back(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\LastVisitedPidlMRU"));
		}
		if(config.getFolderSearch || config.getFileSearch)
		{
			HKCUSubKeyNames.push_back(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\WordWheelQuery"));
		}
	}
	else
	{
		if(config.getRecentDocs)
		{
			HKCUSubKeyNames.push_back(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\OpenSaveMRU\\*"));
		}
		if(config.getRecentlyRun)
		{
			HKCUSubKeyNames.push_back(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ComDlg32\\LastVisitedMRU"));
		}
		if(config.getFileSearch || config.getFolderSearch)
		{
			HKCUSubKeyNames.push_back(_T("Software\\Microsoft\\Search Assistant\\ACMru\\5603"));
			HKCUSubKeyNames.push_back(_T("Software\\Microsoft\\Search Assistant\\ACMru\\5604"));
		}
	}
	if(config.getRecentlyRun)
	{
		HKCUSubKeyNames.push_back(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RunMRU"));
	}
	if(config.getRecentDocs)
	{
		HKCUSubKeyNames.push_back(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\RecentDocs"));
	}
	
	//Node Names
		if(config.getRecentDocs)
		{
			XMLNodeName.push_back(_T("OpenSaveMRU"));
		}
		if(config.getRecentlyRun)
		{
			XMLNodeName.push_back(_T("LastVisitedMRU"));
		}
		if(config.getFileSearch || config.getFolderSearch)
		{
			XMLNodeName.push_back(_T("FolderSearchTerms"));
		}
		if(!((g_osvex.dwMajorVersion == 6) &&(g_osvex.dwMinorVersion==1))) // Windows 7 has a different location
		{
			if(config.getFileSearch || config.getFolderSearch)
			{	
				XMLNodeName.push_back(_T("FileSearchTerms"));
			}
		}
		if(config.getRecentlyRun)
		{	
			XMLNodeName.push_back(_T("RunMRU"));
		}
		if(config.getRecentDocs)
		{
			XMLNodeName.push_back(_T("RecentDocs"));
		}
		
	//Node Names

	//Subnode Names
	if(config.getRecentDocs)
	{
		XMLSubNodeName.push_back(_T("File"));
	}
	if(config.getRecentlyRun)
	{	
		XMLSubNodeName.push_back(_T("Cmd"));
	}
	if(config.getFileSearch || config.getFolderSearch)
	{
		XMLSubNodeName.push_back(_T("Search"));
	}
	if(!((g_osvex.dwMajorVersion == 6) &&(g_osvex.dwMinorVersion==1))) // Windows 7 has a different location
	{
		if(config.getFileSearch || config.getFolderSearch)
		{
			XMLSubNodeName.push_back(_T("Search"));
		}
	}
	if(config.getRecentlyRun)
	{
		XMLSubNodeName.push_back(_T("Cmd"));
	}
	if(config.getRecentDocs)
	{
		XMLSubNodeName.push_back(_T("Docs"));
	}
	
	//Subnode Names

	NameKeyMap[_T("HKEY_CURRENT_USER")]=HKEY_CURRENT_USER;
	
	
}



void WSTRemote::ExtractRegistryInfo(HashedXMLFile &repository,vector<tstring> drives,vector<wstring> extensionList,unsigned long long maxFileSize)
{
	
//1. First open the key in the root key list

	//Open HKCU key
	InitializeHKCURegistryMaps(config);

	int subNameIndex=0;

	
	vector<LPTSTR>::iterator key_it,nodeName;
	map<LPTSTR,HKEY> SubKeyResult;
	
	Registry::extensions = extensionList;
	Registry::maxFileSize=maxFileSize;

	for(key_it=HKCUSubKeyNames.begin(),nodeName=XMLNodeName.begin();key_it<HKCUSubKeyNames.end()&&nodeName<XMLNodeName.end();key_it++,nodeName++)
	{
		WriteNodeToXML(repository,*nodeName,1);//Open
		//cout<<"SubKey Name = "<<*key_it<<endl;
		SubKeyResult[*key_it]=NULL;
		RegOpenKeyEx(NameKeyMap[_T("HKEY_CURRENT_USER")],*key_it,0,KEY_READ,&(SubKeyResult[*key_it]));
		
		//For each of the open subkeys find max key info for the storage of the data returned

		DWORD numSubKeys=0,maxSubKeyLen=0,numValues=0,maxValueNameLen=0,maxValueLen=0;
		FILETIME lastWriteTime;
		RegQueryInfoKey(SubKeyResult[*key_it],NULL,NULL,NULL,&numSubKeys,&maxSubKeyLen,NULL,&numValues,&maxValueNameLen,&maxValueLen,NULL,
						&lastWriteTime);
		//Now allocate the memory
		LPTSTR subKeyName,valueName;
		LPBYTE value;
		subKeyName=(LPTSTR)malloc(sizeof(TCHAR) * (maxSubKeyLen+1));
		valueName=(LPTSTR)malloc(sizeof(TCHAR) * (maxValueNameLen+1));
		value=(LPBYTE)malloc(maxValueLen);
		
		memset(subKeyName,0,sizeof(TCHAR)*(maxSubKeyLen+1));

		//Iterate over all the values printing/adding to XML file for the particular Node
		DWORD index=0;
		for(index=0;index<numValues;index++)
		{
			DWORD valueNameLen,valueLen,valueType;
			LONG result;

			valueNameLen=maxValueNameLen + 1;
			tstring mruValueCheck;
			valueLen=maxValueLen + 1;
			


			memset(value,0,sizeof(BYTE)*maxValueLen);
			memset(valueName,0,sizeof(TCHAR)*(maxValueNameLen+1));
		
			result=RegEnumValue(SubKeyResult[*key_it],index,valueName,&valueNameLen,NULL,&valueType,value,&valueLen);
			mruValueCheck=valueName;

			if(result == ERROR_SUCCESS && mruValueCheck != _T("MRUListEx"))
			{
				
				//printf("value name %s\n",valueName);
				//printf("value %s\n",value);
				//printf("value Len %d\n",valueLen);
				
				WriteValueToXML(value,valueType,valueLen,repository,XMLSubNodeName[subNameIndex],_T("Value"),config);
				//WriteNodeToXML(repository,XMLSubNodeName[subNameIndex],0);
				WriteNodeToXML(repository,_T("Value"),0);
				
			}
			
		}

		free(subKeyName);
		free(valueName);
		free(value);
		
		WriteNodeToXML(repository,*nodeName,0);//Open

		subNameIndex+=1;
	}
	

	TCHAR dirName[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,dirName);
	mruHash<<"</MD5Digest>"<<endl;
	mruHash.close();

	TCHAR g_tempBuf[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,g_tempBuf);
	tstring srcFile = g_tempBuf;
	/*
	if(!MoveFile((srcFile+_T("\\Files\\MRU\\mrudigest_incomplete.xml")).c_str(),(srcFile+_T("\\Files\\MRU\\mrudigest.xml")).c_str()))
	{
		AppLog.Write("MoveFile Failes: mrudigest_incomplete.xml to mrudigest.xml (Try manual rename)");
	}
	*/

}


bool WSTRemote::TraverseRegistry(HKEY hKey,LPCTSTR displayName,bool captureSubKey,HashedXMLFile &repository,LPTSTR KeyName)
{
	// variables needed
	HKEY hSubKey;
	int index;
	//1.Open the key handle - RegOpenKeyEx
	SetLastError(0);
	RegOpenKeyEx(hKey,KeyName,0,KEY_READ,&hSubKey);

	//2. Find MaxSize info regarding the key - RegQueryInfoKey 
	//variables needed
	DWORD numSubKeys,maxSubKeyLen,numValues,maxValueNameLen,maxValueLen;
	FILETIME lastWriteTime;
	RegQueryInfoKey(hSubKey,NULL,NULL,NULL,&numSubKeys,&maxSubKeyLen,NULL,&numValues,&maxValueNameLen,&maxValueLen,NULL,&lastWriteTime);

	//Define and allocate variables for capture of values
	LPTSTR subKeyName = (TCHAR *) malloc((maxSubKeyLen+1) * sizeof(TCHAR));
	memset(subKeyName,0,(maxSubKeyLen+1) * sizeof(TCHAR));
	LPTSTR valueName = (TCHAR *) malloc((maxValueNameLen+1) * sizeof(TCHAR));
	memset(valueName,0,(maxValueNameLen+1) * sizeof(TCHAR));
	LPBYTE value = (BYTE *) malloc((maxValueLen+1));
	memset(value,0,(maxValueLen+1));

	//Two passes 

	//3. 1st Pass - Capture all the values -RegEnumValue
	//variables needed
	DWORD valueNameLen, valueLen, subKeyNameLen,valueType;
	LONG result;
	for(index=0;index<numValues;index++)
	{
		valueNameLen = maxValueNameLen+1;
		valueLen = maxValueLen+1;
		result=RegEnumValue(hSubKey,index,valueName,&valueNameLen,NULL,&valueType,value,&valueLen);
		if(result== ERROR_SUCCESS && GetLastError()==0)
		{
			WriteValueToXML(value,valueType,valueLen,repository,valueName,displayName,config);
			tstring tdisplayName=displayName;
			repository.CloseNode(const_cast<char *>( to_utf8(tdisplayName).c_str()));
		}
	}


	//4. 2nd Pass - SubKeys if the captureSubKey flag is set call TraverseRegistry again.

	if(captureSubKey == true)
	{
		for(index=0;index<numSubKeys;index++)
		{
			subKeyNameLen=maxSubKeyLen+1;
			result=RegEnumKeyEx(hSubKey,index,subKeyName,&subKeyNameLen,NULL,NULL,NULL,&lastWriteTime);
			tstring tsubKeyName=subKeyName;
			repository.OpenNode(const_cast<char *> (to_utf8(tsubKeyName).c_str()));
			
			HKEY hsKey;
			result=RegOpenKeyEx(hKey,subKeyName,0,KEY_READ,&hsKey);
			if(result == ERROR_SUCCESS)
			{
				TraverseRegistry(hsKey,displayName,true,repository);
				string msg="Capturing subkey "+	string((char *)subKeyName);
				AppLog.Write(msg.c_str(),WSLMSG_DEBUG);
			}
			else
			{
				stringstream ss;
				ss<<GetLastError()<<" ";
				
				ss<<"Could not capture subkey "+string((char *)subKeyName);
				string msg=ss.str();
				AppLog.Write(msg.c_str(),WSLMSG_NOTE);
				return false;
			}
		
			RegCloseKey(hsKey);
			repository.CloseNode( const_cast<char *> ( to_utf8(tsubKeyName).c_str()));

		}
	}

	return true;

}


#undef _UNICODE
#undef UNICODE