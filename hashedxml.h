#pragma once

#define _UNICODE
#define UNICODE

#include <fcntl.h>
#include <io.h>
#include "stdafx.h"

extern "C" {
#include "md5.h"
}

using namespace std;


class HashedXMLFile 
{
    FILE *m_pEvidenceFile;
    MD5_CTX m_md5Data;
    bool m_bNewLine;
    unsigned char m_hash[20];
    //tstring m_szXMLFilename;
    bool m_bCompress;
public:
    HashedXMLFile()
    {
        m_bNewLine=false;
        m_pEvidenceFile = NULL;
    };
    int Open(char *szFilename); //, bool bCompress);
    int Close(unsigned char *pHash);
    void OpenNode(char *szName);
    void OpenNodeWithAttributes(char *szName);
    void CloseNode(char *szName);

	void Write(const wchar_t *szName);
    void Write(const char *szName);

	void WriteNodeWithValue(char *szName, const wchar_t *szValue);
    void WriteNodeWithValue(char *szName, const char *szValue);

	void WriteAttribute(char *szName, const wchar_t *szValue);
    void WriteAttribute(char *szName, const char *szValue);

    int WriteHash(char *szFilename);

};

