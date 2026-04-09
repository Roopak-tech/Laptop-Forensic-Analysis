
//#define _AFXDLL //-Enable for debug mode 
#include <afx.h>
#include <iostream>
//#include <afxwin.h>
#include <windows.h>
#include <process.h>
#include "../WSTRemote.h"
#include <string>
#include <vector>
#include <map>
#include <fstream>

#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <Windows.h>
#include <AclAPI.h>


#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>


using namespace std;
//extern string escapeXML(string);

string escapeXML(string str)
{
	char findStr[]={'&','<','>','\0'};
	string escapedString=str;
	int index=0;
	int jump=0;

	while(((index=escapedString.find_first_of(findStr,jump))!=escapedString.npos))
	{
		char c=(escapedString.c_str())[index];
		jump=0;
		
		if(c=='&')
		{
			escapedString = escapedString.substr(0,index)+"&amp;"+escapedString.substr(index+1);
			jump=index+4;
		}
		else if(c=='<')
		{
			escapedString = escapedString.substr(0,index)+"&lt;"+escapedString.substr(index+1);
			jump=index+3;
		}
		else if(c=='>')
		{
			escapedString = escapedString.substr(0,index)+"&gt;"+escapedString.substr(index+1);
			jump=index+3;
		}
			
	}
	return escapedString;

}

static const size_t MD5HashByteCount = 16;
using namespace std;
ofstream fd;
//ofstream fdr;
ofstream fdHash;
WSTRemote *rAgent;
ofstream flog;
int abortFlag=0;
typedef struct searchParams
	{
		vector<string> drives;
		map<string,int> extensions;
		string lattDrive;
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
		   cout<<"Hello"<<endl;
		   lattDrive.clear();
		}
		
		


	}searchParams;


map<string,int> extensionList;
unsigned long long maxFileSize;
int previousDays;
SYSTEMTIME acqTime;
int stcount;
DWORD parentThreadId;

#define FILE_FOUND WM_APP+0x0001

HANDLE copyMutex;
HANDLE abortMutex;
vector<string> filesToCopy;


/*FileHash Code */


int MD5Hash(tstring filename, BYTE *hashVal)
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
	string fileName(filename.begin(), filename.end());
	err = _tfopen_s(&pFile, fileName.c_str(), TEXT("rb"));
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



void CloseFileHandles(ofstream &flog)
{

	if(flog.is_open())
	{
		flog.close();
	}
}

void addP()
{
	flog<<"<MSG Type=\"STATUS\"> ";
}

void addE()
{
	flog<<" </MSG> "<<endl;
}

string GetOwnerInfo(string filename)
{

	PSID owner=NULL;
	PSECURITY_DESCRIPTOR psd=NULL;
	DWORD retval;

	retval=GetNamedSecurityInfo(LPWSTR(filename.c_str()),SE_FILE_OBJECT,OWNER_SECURITY_INFORMATION,&owner,NULL,NULL,NULL,&psd);

	if(retval !=0)
	{
		flog<<"GetNamedSecurityInfo "<<GetLastError()<<endl;
		return "";
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

		if(!LookupAccountSid(NULL,owner,fileOwner,&length,dname,&dlength,&su))
		{
			flog<<"LookupAccountSid "<<GetLastError()<<endl;
			LocalFree(psd);
			return "";
		}
		else
		{
			LocalFree(psd);
			return string(fileOwner);
		}

	}


}
namespace FileSearch
{
		struct fileSearch
		{
				string drive;
				string extension;
				int createdDate;
				int modifiedDate;

		};

		void Recurse(string,string,int,int);

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

				//flog<<pattern->drive<<endl;
				//flog<<pattern->extension<<endl;
				addP();
				flog<<" Thread: Starting new search thread for "<<pattern->drive;
				addE();

				Recurse(pattern->drive,pattern->extension,pattern->createdDate,pattern->modifiedDate);
				//Recurse(pattern->drive,pattern->extension);
				fflush(stdout);
				addP();
				flog<<"Thread : Ending search thread for"<<pattern->drive;
				addE();
				_endthreadex(0);
				return 0;

		}

		//Copy constraint functions
		
		//1. File type

		bool inExtensionList(string filename)
		{
				return !(extensionList.find(filename.substr(filename.find_last_of('.')+1)) == extensionList.end());
		}

		//2. File size
		bool isWithinFileSize(unsigned long long fileSize)
		{
				return (fileSize <= maxFileSize);
		}

		//time check
		BOOL dateCheck(FILETIME ftime)
		{
			SYSTEMTIME fileTime;
			FileTimeToSystemTime(&ftime,&fileTime);

			if(fileTime.wYear==acqTime.wYear)
			{
				if(fileTime.wMonth==acqTime.wMonth)
				{
					if((acqTime.wDay - fileTime.wDay) <= previousDays)
					{
						return TRUE;
					}
				}
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
					return (dateCheck(ctime));
				case 2:
					return (dateCheck(ftime));
				}
		}

		void Recurse(string drive,string extension,int createdDate,int modifiedDate)
		{
				CFileFind finder;
				CString pattern=(drive+"\\*"+extension).c_str();
				//flog<<(drive+"\\*"+extension).c_str()<<endl;

				BOOL findFirst=finder.FindFile(pattern);
				DWORD waitResult;
				//flog<<(int)findFirst<<endl;
				DWORD abortMutexRet;

				while(findFirst)
				{
					abortMutexRet=WaitForSingleObject(abortMutex,INFINITE);
						switch(abortMutexRet)
						{	
							case WAIT_OBJECT_0:
								if(Aborting())
								{
									//__try									
									
										if(!ReleaseMutex(abortMutex))
										{
											addP();
											flog<<"Recurse :"<<"Error releasing abortMutex mutex";
											flog<<" "<<GetLastError();
											addE();
											_endthreadex(-1);
										}
										else
									//__finally
										{
											_endthreadex(0);
										}
								}
								else
								{
										if(!ReleaseMutex(abortMutex))
										{
											
											addP();
											flog<<"Recurse :"<<"Error releasing abortMutex mutex";
											flog<<" "<<GetLastError();
											addE();
										}
								}
								break;
							case WAIT_ABANDONED:
								addP();
								flog<<"Recurse :"<<"Error abortMutex Abandoned "<<GetLastError();
								addE();
								//CloseFileHandles(flog);
								//_endthreadex(-1);
								break;
							case WAIT_FAILED:
								addP();
								flog<<"Recurse :"<<"Error abortMutex Failed "<<GetLastError();
								addE();
								//CloseFileHandles(flog);
								//_endthreadex(-1);
						}
						findFirst=finder.FindNextFile();

						if(finder.IsDots())
						{
							//flog<<"Recurse : Skipping . and .."<<endl;
								continue;
						}


						if(finder.IsDirectory())
						{
								string dir=finder.GetFilePath();
//								//flog<<dir.c_str()<<endl;
								Recurse(dir,extension,createdDate,modifiedDate);
								
						}
						else
						{
								string fileName=finder.GetFileName();
								unsigned long long fileSize= finder.GetLength();
								FILETIME fileWriteTime;
								FILETIME createdTime;
								finder.GetLastWriteTime(&fileWriteTime);
								finder.GetCreationTime(&createdTime);
								if(inExtensionList(fileName) && isWithinFileSize(fileSize) && isWithinLastPreviousDays(fileWriteTime,createdTime,createdDate,modifiedDate))
								{
										waitResult=WaitForSingleObject(copyMutex,INFINITE);
										string filepath;

										switch(waitResult)
										{
												case WAIT_OBJECT_0:
												//	try
												//__try
													{
													filepath=finder.GetFilePath();
													//flog<<"Recurse : "<<filepath.c_str()<<endl;
													/*
													fdr<<"<File>";
													fdr<<"<Path>";
													//(rAgent->threadXMLHandle).OpenNode("File");
													fdr<<filepath;
													fdr<<"</Path>"<<endl;
													fdr<<"</File>"<<endl;
													*/
													filesToCopy.push_back(filepath);
													//PostThreadMessage(parentThreadId,WM_USER+0x0000,0L,0L);
													}
												//finally
												//__finally
												{
													if(!ReleaseMutex(copyMutex))
													{
															addP();
														flog<<"Recurse :Error releasing copyMutex mutex "<<GetLastError();
														addE();
													//	CloseFileHandles(flog);
														_endthreadex(-1);
													}
												}
													break;
												case WAIT_ABANDONED:
													addP();
													flog<<"Recurse :Error CopyMutex Abaondoned "<<GetLastError();
													addE();
													//CloseFileHandles(flog);
													_endthreadex(-1);
													return;

												case WAIT_FAILED:
													addP();
													flog<<"Recurse :Error CopyMutex Failed"<<GetLastError();
													addE();
													//CloseFileHandles(flog);
													_endthreadex(-1);
													return;
										}

								}

						}


				}

				finder.Close();
		}

		//Return the destination directory after creating it.
		string mkdirp(string srcFileName,string dstDrive)
		{
			int index,cIndex;

			string tempFileName=srcFileName;
			string dirName=dstDrive;

			while((index=tempFileName.find_first_of("\\"))!=string::npos)
			{
					string newDir=tempFileName.substr(0,index);

					if((cIndex=newDir.find_first_of(":"))!=string::npos)
					{
							newDir=newDir.substr(0,cIndex);
					}
					dirName = dirName +"\\"+newDir;
					tempFileName=tempFileName.substr(index+1);
					/*
					addP();
					flog<<"mkdirp : Creating directory "<<dirName.c_str()<<endl;
					addE();
					*/
					/*
					if(!CreateDirectory(dirName.c_str(),NULL))
					{
							if(GetLastError() != ERROR_ALREADY_EXISTS)
							{
									addP();
									flog<<"mkdirp : Error creating directory"<<GetLastError();
									addE();
									return " ";
							}
					}
					*/
				
			}
			addP();
			flog<<"mkdirp: File destination returned "<<(dirName+"\\"+tempFileName).c_str();
			addE();
			return dirName+"\\"+tempFileName;
			
		}


		unsigned __stdcall copyThread(void *arglist)
		{

				
				string *dstdir=0;
				dstdir=(string *)arglist;
				
				DWORD mutexState;
				//flog<<"copyThread "<<stcount<<endl;
				MSG msg;
				PeekMessage(&msg,NULL,WM_USER,WM_USER,PM_NOREMOVE);

				DWORD abortMutexRet=WaitForSingleObject(abortMutex,INFINITE);
				switch(abortMutexRet)
				{	
					case WAIT_OBJECT_0:
							if(Aborting())
							{
									//__try
									//try
									//{
											if(!ReleaseMutex(abortMutex))
											{
													addP();
													flog<<"copyThread :"<<"Error releasing abortMutex mutex";
													flog<<" "<<GetLastError();
													addE();
													filesToCopy.clear();
													_endthreadex(-1);
											}
											else
											{
												filesToCopy.clear();
												_endthreadex(0);
											}

									//}
									//__finally
									//finally
									
							}
							else
							{
									if(!ReleaseMutex(abortMutex))
									{
											addP();
											flog<<"copyThread :"<<"Error releasing abortMutex mutex";
											flog<<" "<<GetLastError();
											addE();
									}
							}
						break;
					case WAIT_ABANDONED:
						addP();
						flog<<"copyThread: Error abortMutex Abandoned "<<GetLastError();
						addE();
						//CloseFileHandles(flog);
						//_endthreadex(-1);
						//return 1;
						break;
					case WAIT_FAILED:
						addP();
						flog<<"copyThread:Error abortMutex Failed "<<GetLastError();
						addE();
						//CloseFileHandles(flog);
						//_endthreadex(-1);
						//return 2;
						break;
					
				}
				while(stcount||filesToCopy.size())
				{
						mutexState=WaitForSingleObject(copyMutex,INFINITE);
						switch(mutexState)
						{
								case WAIT_OBJECT_0:
										if(filesToCopy.size()==0)
										{
												if(!ReleaseMutex(copyMutex))
												{
														addP();
														flog<<"copyThread :Error releasing copyMutex "<<GetLastError();
														addE();
														//CloseFileHandles(flog);
														_endthreadex(-1);
												}
												break;
										}
										else
										{
												//int fIndex=(filesToCopy.back()).find_last_of("\\");
												//string dstFileName=(*dstdir)+"\\"+(filesToCopy.back()).substr(fIndex+1);
												
												try
												{
													string srcFileName=filesToCopy.back();

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
													string extension=srcFileName.substr(srcFileName.find_last_of('.')+1);
													dstFileName<<"Files\\Collected\\"<<"File"<<ss.str().c_str()<<"."<<extension;
													string dstPath=mkdirp(srcFileName,*dstdir);
													

													if(dstFileName.str() == " ")
													{
														addP();
														flog<<"copyThread :Error Skipping copy, could not create directory ";
														addE();
														//CloseFileHandles(flog);
														break;
													}	
													rAgent->currentState=CAPTURING_RECENT_FILES;
													addP();
													flog<<"copyThread :CAPTURING_RECENT_FILES ";
													addE();
													rAgent->fileName=srcFileName;
													rAgent->filesProcessed++;
													//flog<<"copyThread:dst src"<<dstFileName<< " "<<srcFileName<<endl;
													filesToCopy.pop_back();


													unsigned char md5hash[20];
													memset(md5hash,0,20);
													wstring fileName(srcFileName.begin(), srcFileName.end());
													MD5Hash(fileName.c_str(), md5hash);
													fdHash<<"<File>"<<endl;
													fdHash<<"<CopiedFileName> "<<dstFileName.str().c_str()<<"</CopiedFileName>"<<endl;
													fdHash<<"<OriginalFileName> "<<escapeXML(srcFileName).c_str()<<"</OriginalFileName>"<<endl;
													fdHash<<"<Hash> ";
													for (int i = 0; i < 16; ++i)
													{
														char piece[2];
														memset(piece,0,2);
														sprintf(piece,"%02X",md5hash[i]);
														fdHash<<piece;
														memset(piece,0,2);
													}
													//GetFileAttributes(
													fdHash<<"</Hash>"<<endl;
													/* stat information of the acquired files */	
													/*
													struct __stat64 finfo;
													int result=_stat64(srcFileName.c_str(),&finfo);
													if(result == 0)
													{
														string owner;
														owner.clear();
														fdHash<<"<FileSize>"<<finfo.st_size<<" Bytes"<<"</FileSize>"<<endl;
														fdHash<<"<ModifiedTime>"<<_ctime64(&(finfo.st_mtime))<<"</ModifiedTime>"<<endl;
														fdHash<<"<CreatedTime>"<<_ctime64(&(finfo.st_ctime))<<"</CreatedTime>"<<endl;
														fdHash<<"<AccessTime>"<<_ctime64(&(finfo.st_atime))<<"</AccessTime>"<<endl;
													
														owner=GetOwnerInfo(dstFileName.str());
														if(owner != "")
														{
															fdHash<<"<Owner>"<<owner.c_str()<<"</Owner>"<<endl;
														}
													
													}
													else
													{
														flog<<"Could not stat file "<<dstFileName.str().c_str()<<" "<<GetLastError()<<endl;
													}
													*/
													/* stat information of the acquired files */
													fdHash<<"</File>"<<endl;


													if(!CopyFile(srcFileName.c_str(),dstFileName.str().c_str(),FALSE))
													{
														addP();
														flog<<"copyThread: Error Could not copy File "<<GetLastError();
														addE();
													}
													if(!ReleaseMutex(copyMutex))
													{
															addP();
														flog<<"copyThread: Error releasing mutex in copy "<<GetLastError();
														addE();
														 //CloseFileHandles(flog);
														_endthreadex(-1);
													}

												}
												catch(...)
												{
													if(!ReleaseMutex(copyMutex))
													{
															addP();
														flog<<"copyThread: Error releasing mutex in copy "<<GetLastError();
														addE();
														 //CloseFileHandles(flog);
														_endthreadex(-1);
													}
												}
												break;
										}
								case WAIT_ABANDONED:
										addP();
										flog<<"copyThread: Error copythread abandoned "<<GetLastError();
										addE();
										//CloseFileHandles(flog);
										_endthreadex(-1);
										return 1;
								case WAIT_FAILED:
										addP();
										flog<<"copyThread: Error copythread failed "<<GetLastError();
										addE();
										//CloseFileHandles(flog);
										_endthreadex(-1);
										return 2;


						}
				}
				addP();
				flog<<"Ending Copy thread";
				addE();


				_endthreadex(0);
				return 0;
		}


		int searchMultipleTargets(searchParams sp,WSTRemote *w)
		{
			
				DWORD exitCode=0;

				

			
				DWORD threadCount=sp.drives.size();//One for the copy thread
				if(threadCount == 0)
				{
					
				//	__asm int 3;
					return 0;
				}
				DWORD threadIndex=0;
				int iThread=0;
				int i=0;
				GetSystemTime(&acqTime);

				HANDLE *tHandle=NULL;
				fileSearch *search=NULL;
				DWORD exitCopyThread=1;

				tHandle=(HANDLE *) malloc(sizeof(HANDLE)*threadCount);
				search=(fileSearch *) malloc(sizeof(fileSearch)*threadCount);
				extensionList=sp.extensions;
				maxFileSize=sp.maxFileSize;
				previousDays=sp.previousDays;
				
				fdHash.open("filedigest.xml");
				fdHash<<"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
				fdHash<<"<?xml-stylesheet type=\"text/xsl\" href=\"filedigest.xslt\"?>\n";
				fdHash<<"<MD5Digest>"<<endl;

				flog.open("filecaplog.xml");
				flog<<"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
				flog<<"<FileCaptureLog>"<<endl;

				addP();
				flog<<"In searchMultipleTargets";
				addE();
				addP();
				flog<<"Scanning Drives ";
				addE();

				addP();
				flog<<"Drives being scanned are: ";
				for(vector<string>::iterator it=sp.drives.begin();it<sp.drives.end();it++)
				{
					flog<<(*it).c_str()<<" ";
				}
				addE();

				parentThreadId=GetCurrentThreadId();
				addP();
				flog<<"Main: Current Thread ID "<<parentThreadId;
				flog<<"Main: Current State"<<w->currentState;
				addE();

				rAgent=w;


				/*fdr.open("files.xml");
				fdr<<"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
				fdr<<"<RecentlyModifiedFiles>"<<endl;
				*/
				

				copyMutex=CreateMutex(NULL,FALSE,NULL);

				if(copyMutex==NULL)
				{
						addP();
					flog<<"Main: Error Failed to create copyMutex";
					addE();
					//CloseFileHandles(flog);
					_endthreadex(-1);

				}
				abortMutex=CreateMutex(NULL,FALSE,NULL);

				if(abortMutex==NULL)
				{
						addP();
					flog<<"Main: Error Failed to create abortMutex";
					addE();
					//CloseFileHandles(flog);
					_endthreadex(-1);

				}

				//flog<<"After creating Mutex"<<endl;

				memset(tHandle,0,sizeof(HANDLE)*threadCount);
				memset(search,0,sizeof(fileSearch)*threadCount);
				i=0;
				for(vector<string>::iterator dit=sp.drives.begin();dit<sp.drives.end();dit++)
				{
						search[i].drive=*dit;
						addP();
						flog<<"Main :"<<(*dit).c_str();
						addE();
						search[i].extension=".*";
						search[i].createdDate=sp.createdDate;
						search[i].modifiedDate=sp.modifiedDate;
						//tHandle[i]=INVALID_HANDLE_VALUE;
						tHandle[i]=(HANDLE)_beginthreadex(NULL,0,Thread,(void *)(&(search[i])),0,NULL);
						if(tHandle[i] == 0)
						{
							addP();
							flog<<"Main: Error creating thread "<<i<<" "<<GetLastError();
							addE();
							//CloseFileHandles(flog);
							_endthreadex(-1);
						}
						{
							addP();
							flog<<"Main: "<<i<<"Thread successful";
							addE();
						}
						//flog<<"i: "<<i<<endl;
						i++;
				}
				addP();
				flog<<"Main:Spawned "<<i<<" threads";
				addE();

				stcount=threadCount;

				//Copy thread
				HANDLE copyHandle=INVALID_HANDLE_VALUE;	
				unsigned int copyID=0;
				
				copyHandle=(HANDLE)_beginthreadex(NULL,0,copyThread,(void *)(&(sp.lattDrive)),0,&copyID);
				if(copyHandle == 0)
				{
					addP();	
					flog<<"Main: Error creating copy thread,freeing resources and exiting... "<<GetLastError();
					addE();	
					for(int i=0;i<threadCount;i++)
					{
						CloseHandle(tHandle[i]);
					}
					free(tHandle);
					//CloseFileHandles(flog);
					_endthreadex(-1);
				}
				else
				{
					addP();
					flog<<"Main: Spawned search thread ";
					addE();
				}
				

				while(threadCount > 0)
				{
						threadIndex = WaitForMultipleObjects(threadCount,tHandle,FALSE,INFINITE);

						if(threadIndex == -1)
						{
								addP();
							flog<<"Main: Error Wait failed (in waiting for threads), freeing resources and exiting... "<<GetLastError();
							addE();
							for(int i=0;i<threadCount;i++)
							{
								CloseHandle(tHandle[i]);
							}
							CloseHandle(copyHandle);
							free(search);
							free(tHandle);

							_endthreadex(-1);

						}
						iThread = (int) threadIndex - (int)WAIT_OBJECT_0;
						addP();
						flog<<"Wait over for thread "<<iThread;
						addE();
						GetExitCodeThread(tHandle[iThread],&exitCode);
						if(tHandle[iThread] != INVALID_HANDLE_VALUE)
						{
							//flog<<"Before Closing thread"<<endl;
							if(!CloseHandle(tHandle[iThread]))
							{
								addP();
								flog<<"Main: Error (CloseHandle): "<<GetLastError();
								addE();
								_endthreadex(-1);
							}
							tHandle[iThread]=tHandle[threadCount - 1]; //Very Important
							threadCount--;
							stcount--;
								addP();
							flog<<"Main: iThread  exitCode threadcode threadIndex"<<iThread<<" " <<exitCode<<" "<<threadCount<<" "<<threadIndex;
								addE();
						}
						else
						{
								addP();
							flog<<"Main :Error Invalid Handle -  exitCode not 0 "<<exitCode<<GetLastError();
								addE();
							_endthreadex(-1);
						}

				}
				if(tHandle != NULL)
				{
					free(tHandle);
								addP();
					flog<<"Main :Freeing memory for tHandle";
								addE();
				}
				if(search != NULL)
				{
					free(search);
								addP();
					flog<<"Main :Freeing memory for search";
								addE();
				}

				for(;;)
				{
					DWORD abortMutexRet=WaitForSingleObject(abortMutex,INFINITE);
					switch(abortMutexRet)
					{	
						case WAIT_OBJECT_0:
								if(Aborting())
								{
								addP();
									flog<<"Main: Aborting abortMutex";
								addE();
									if(!ReleaseMutex(abortMutex))
									{

								addP();
										flog<<"Main :"<<"Error releasing abortMutex mutex";
										flog<<" "<<GetLastError();
								addE();
										_endthreadex(-1);
									}

								}
								else
								{
								addP();
										flog<<"Main: Releasing abortMutex";
								addE();
										if(!ReleaseMutex(abortMutex))
										{
								addP();
												flog<<"Main :"<<"Error releasing abortMutex mutex"<<endl;
												flog<<" "<<GetLastError();
								addE();
										}
								}
								break;
						case WAIT_ABANDONED:
								addP();
							flog<<"Main :"<<"Error abortMutex Abandoned "<<GetLastError();
								addE();
							//CloseFileHandles(flog);
							//_endthreadex(-1);
							break;
						case WAIT_FAILED:
								addP();
							flog<<"Main :"<<"Error abortMutex Failed "<<GetLastError();
								addE();
							//CloseFileHandles(flog);
							//_endthreadex(-1);
							break;
					}
					if(!threadCount && (filesToCopy.size()==0))
					{
								addP();
						flog<<"Main   : Killing copyThread (we are done not an error)"<<endl;
								addE();

						BOOL retCode;
						
						retCode=PostThreadMessage(copyID,WM_USER+1010,0L,0L);
						while(retCode != 0)
						{
							Sleep(2000);
							retCode=PostThreadMessage(copyID,WM_USER+1010,0L,0L);
							addP();
							flog<<"PostThreadMessage to CopyThread Failed: "<<GetLastError();
							addE();
						}
						
					}
					DWORD retVal;
					retVal=WaitForSingleObject(copyHandle,INFINITE);

					if(retVal==WAIT_OBJECT_0)
					{
								addP();
						flog<<"Main : thread count "<<threadCount;
								addE();
								addP();
						flog<<"Main files to copy "<<filesToCopy.size();
								addE();

						if(abortFlag)
						{
							rAgent->currentState=ABORTING;
						}

						if(copyHandle != INVALID_HANDLE_VALUE)
						{
							CloseHandle(copyHandle);
						}
						/*
						if(fdr)
						{
							fdr<<"</RecentlyModifiedFiles>"<<endl;
							fdr.close();
								addP();
							flog<<"Main: Closed files.xml";
								addE();
						}
						*/
						if(abortMutex != INVALID_HANDLE_VALUE)
						{
							CloseHandle(abortMutex);
								addP();
							flog<<"Main: Closed abortMutex ";
								addE();
						}
						if(copyMutex != INVALID_HANDLE_VALUE)
						{
							CloseHandle(copyMutex);
								addP();
							flog<<"Main : Closed copyMutex ";
								addE();
						}

						if(fdHash)
						{
								addP();
							flog<<"Main : After all the threads, closing digest file";
								addE();
							fdHash<<"</MD5Digest>"<<endl;
							

							CloseFileHandles(fdHash);
						}
						if(flog)
						{
								addP();
							flog<<"Main : After all the threads, closing log file";
								addE();
							//fdr<<"</FileCaptureLog>"<<endl;
							flog<<"</FileCaptureLog>"<<endl;
							rAgent->currentState=CAPTURE_COMPLETE;
							//flog<<"Main: Current State"<<w->currentState;
								
							CloseFileHandles(flog);
						}

						break;
					}
				}

/*
				_CrtSetReportMode(_CRT_ERROR,_CRTDBG_MODE_DEBUG);


				return 0;
				_CrtDumpMemoryLeaks();
				*/
				_endthreadex(0);


			
				

		}


}
