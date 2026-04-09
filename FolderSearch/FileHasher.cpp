//#include "stdafx.h"
#include "FileHasher.h"

FileHasher::FileHasher(void)
{
}

FileHasher::~FileHasher(void)
{
}

int FileHasher::MD5HashFile(tstring filename, BYTE *hashVal)
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

tstring FileHasher::MD5BytesToString(BYTE *MD5Hash)
{

	TCHAR buf[MD5HashByteCount * 2 + 1];   // each byte will take up 2 hex digits in the string
	tstring result;

	for (int i=0; i < MD5HashByteCount; i++) {		
		_stprintf_s(buf + i * 2, 3, TEXT("%2.2X"), MD5Hash[i]);
	}

	result = buf;
	return result;

}