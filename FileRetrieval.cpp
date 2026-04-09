#include "FileRetrieval.h"

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <direct.h>
#include <wchar.h>

#include <vector>
#include <string>
#include <iostream>

using namespace std;

unsigned int FindFilesWST(const TCHAR *FilePath, const TCHAR *FileMask, const int Recurse, WSTRemote *pRemote)
{
	_tfinddata_t fileinfo;
	intptr_t hFindHandle;
	unsigned int FileCount = 0;
	unsigned int FullSearch = 0;
	TSTRING TempPath = FilePath;
	TSTRING CurrentDir;
	vector<TSTRING> Masks;
	TCHAR *TempString = NULL;
	TCHAR *ReverseMask = NULL;
	TCHAR *Token = NULL;

	TempString = _tgetcwd(NULL, 1);
	CurrentDir = TempString;
	free(TempString);
	TempString = NULL;

	if(FileMask != NULL)
	{
		TempString = _tcsdup(FileMask);
		
		Token = _tcstok(TempString, _T(";"));

		while(Token != NULL)
		{
			Masks.push_back(Token);
			Token = _tcstok(NULL, _T(";"));
		}
	}

	if(TempPath[TempPath.length() - 1] != _T('\\'))
		TempPath += _T("\\");

	if((hFindHandle = _tfindfirst((TempPath + _T("*.*")).c_str(), &fileinfo)) == -1L)
	{
		printf("Could not find any matching files.\n");
		return 0;
	}

	do
	{
		if(pRemote->currentState != ABORTING)
			pRemote->currentState = COPYING_FILES;
		else
			goto cleanup;


		// Ignore . and ..
		if(!_tcscmp(_T("."), fileinfo.name) || !_tcscmp(_T(".."), fileinfo.name))
			continue;

		// Check to see if this is a directory & we want to recurse
		if((fileinfo.attrib & _A_SUBDIR) && Recurse)
		{
			// We need to skip "ourself" so we don't get into a nasty recursive loop...
			// TODO: Need to put in a check for root directories
			if(_tcsnicmp(CurrentDir.c_str(), (TempPath + fileinfo.name).c_str(), CurrentDir.length()) == 0)
				continue;

			FileCount += FindFilesWST((TempPath + fileinfo.name).c_str(), FileMask, Recurse, pRemote);
		}
		else // We're looking at a file
		{
			// Are we filtering file masks?
			if(Masks.size() > 0)
			{
				// This is where we need to check the file extension
				TempString = _tcsdup(fileinfo.name);
				_tcslwr(TempString);
				_tcsrev(TempString);

				for(unsigned int i = 0; i < Masks.size(); i++)
				{
					
					// This searches the entire file name for a match...
					if(FullSearch)
					{
						if(_tcsstr(TempString, Masks[i].c_str()) != NULL)
						{
							_tprintf(_T("Filename: %s\n"), (TempPath + fileinfo.name).c_str());
							FileCount++;
							break;
						}
					}
					else
					{
						// This searches just the extension... Tricky!
						ReverseMask = _tcsdup(Masks[i].c_str());
						_tcslwr(ReverseMask);

						if(_tcsnicmp(TempString, _tcsrev(ReverseMask), Masks[i].length()) == 0)
						{
							if(!CopyFile_Plugin((TempPath + fileinfo.name).c_str(), CurrentDir.c_str()))
							{
								_tprintf(_T("Could not copy: %s\n"), (TempPath + fileinfo.name).c_str());
							}
							
							FileCount++;
							free(ReverseMask);
							ReverseMask = NULL;
		
							pRemote->filesProcessed++;

							break;
						}

						free(ReverseMask);
						ReverseMask = NULL;
					}
				}
			
				free(TempString);
				TempString = NULL;
			}
			else	// We're going to pull every single file.
			{
				if(!CopyFile_Plugin((TempPath + fileinfo.name).c_str(), CurrentDir.c_str()))
				{
					_tprintf(_T("Could not copy: %s\n"), (TempPath + fileinfo.name).c_str());
				}

				FileCount++;
			}
		}
	} while(_tfindnext(hFindHandle, &fileinfo) == 0);

cleanup:

	_findclose(hFindHandle);

	return FileCount;
}

bool CopyFile_Plugin(const TCHAR *FileSource, const TCHAR *RootDestination)
{
	FILE *SourceFile = NULL;
	FILE *DestinationFile = NULL;
	TCHAR *Token = NULL;
	TCHAR *SourcePath = _tcsdup(FileSource);
	TCHAR *TempString = NULL;
	TSTRING DestinationPath = RootDestination;
	TSTRING OriginalDir = _T("");
	unsigned char *Buffer = NULL;
	size_t BufferSize = 16384;
	size_t BytesRead = 0;
	size_t StringLength = 0;
	unsigned int TokenCount = 0;
	bool ReturnValue = false;

	// Save the current working directory so we can clean up after ourselves
	TempString = _tgetcwd(NULL, 1);
	OriginalDir = TempString;
	free(TempString);
	TempString = NULL;

	// Add a trailing backslash if needed
	if(DestinationPath.find(_T("RetrievedFiles")) == TSTRING::npos)
	{
		if(DestinationPath[DestinationPath.size() - 1] != _T('\\'))
			DestinationPath = DestinationPath + _T("\\RetrievedFiles");
		else
			DestinationPath = DestinationPath + _T("RetrievedFiles");
	}

	if(_tchdir(DestinationPath.c_str()) != 0)
	{
		if(_tmkdir(DestinationPath.c_str()) != 0)
		{
			_tprintf(_T("Could not create repository path: %s"), DestinationPath.c_str());
			goto cleanup;
		}
	}

	// Need to parse the path and create the directory structure if it doesn't exist
	Token = _tcstok(SourcePath, _T("\\:/"));
	
	while(Token != NULL)
	{
		// Keep track of how much of the string has been processed so we
		// don't create the filename as a directory.
		TokenCount++;
		StringLength += _tcslen(Token);

		if(StringLength + TokenCount >= _tcslen(FileSource))
			break;

		// Create the file directory structure
		DestinationPath += _T("\\");
		DestinationPath += Token;

		// Does this path exist?
		if(_tchdir(DestinationPath.c_str()) != 0)
		{
			// Could not make the directory
			if(_tmkdir(DestinationPath.c_str()) != 0)
			{
				_tprintf(_T("Could not create directory: %s\n"), DestinationPath.c_str());
				goto cleanup;
			}
		}

		Token = _tcstok(NULL, _T("\\:/"));
	}

	DestinationPath += _T("\\");

	// At this point our directory tree should be set properly, so we can hypothetically
	// copy the file there.
	if((SourceFile = _tfopen(FileSource, _T("rb"))) == NULL)
	{
		_tprintf(_T("Could not open source file for reading.\n"));
		goto cleanup;
	}

	//if((DestinationFile = _tfopen((DestinationPath + Token).c_str(), _T("wb"))) == NULL)
	if((DestinationFile = _tfopen((DestinationPath + Token).c_str(), _T("wb"))) == NULL)
	{
		_tprintf(_T("Could not open destination file for writing.\n"));
		goto cleanup;
	}

	Buffer = (unsigned char*)malloc(sizeof(unsigned char) * BufferSize);

	if(Buffer == NULL)
	{
		_tprintf(_T("Could not create buffer.\n"));
		goto cleanup;
	}

	while(!feof(SourceFile))
	{
		BytesRead = 0;
		memset(Buffer, 0, sizeof(unsigned char) * BufferSize);

		BytesRead = fread(Buffer, sizeof(unsigned char), BufferSize, SourceFile);

		if(!ferror(SourceFile) && (BytesRead > 0))
		{
			if(fwrite(Buffer, sizeof(unsigned char), BytesRead, DestinationFile) != BytesRead)
			{
				_tprintf(_T("Could not write to file.\n"));
				goto cleanup;
			}
		}
		else
		{
			
		}
	}

	ReturnValue = true;

cleanup:
	_tchdir(OriginalDir.c_str());

	if(SourcePath != NULL)
		free(SourcePath);

	if(SourceFile != NULL)
		fclose(SourceFile);

	if(DestinationFile != NULL)
		fclose(DestinationFile);

	if(Buffer != NULL)
		free(Buffer);

	return ReturnValue;
}