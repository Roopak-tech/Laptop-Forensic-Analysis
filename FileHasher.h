#pragma once

#ifdef WIN32
#include "stdafx.h"
#endif

#define _UNICODE
#define UNICODE

#include "tstl.h"


extern "C" {
#include "md5.h"
}

using namespace std;

class FileHasher
{

public:

	static const size_t MD5HashByteCount = 16;
	static const size_t SHA1HashByteCount = 20;

	FileHasher(void);
	~FileHasher(void);

	int MD5HashFile(tstring filename, BYTE *hashVal);

	tstring MD5BytesToString(BYTE *MD5Hash);

};
