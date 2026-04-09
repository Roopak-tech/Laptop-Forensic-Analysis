config.getLiveFileSystems = true;
// HashedXML.cpp : Defines the entry point for the console application.
//
#include "winerror.h"
#include "stdafx.h"
#include "hashedxml.h"

//*************************************************************************************
//
//  OpenNodeWithAttributes
//
//*************************************************************************************
void HashedXMLFile::OpenNodeWithAttributes(char *szName)
{
    string szString;
    if (!m_bNewLine)
       szString = "\n";
    szString.append("<");
    szString.append(szName);
	_ftprintf_s(m_pEvidenceFile, szString.c_str());
    MD5Update(&m_md5Data, (unsigned char *)szString.c_str(), szString.length());
    m_bNewLine = false;
}

//*************************************************************************************
//
//  OpenNode
//
//*************************************************************************************
void HashedXMLFile::OpenNode(char *szName)
{
    string szString;
    if (!m_bNewLine)
       szString = "\n";
    szString.append("<");
    szString.append(szName);
    szString.append(">");
	_ftprintf_s(m_pEvidenceFile, szString.c_str());
    MD5Update(&m_md5Data, (unsigned char *)szString.c_str(), szString.length());
    m_bNewLine = false;
}

//*************************************************************************************
//
//  CloseNode
//
//*************************************************************************************
void HashedXMLFile::CloseNode(char *szName)
{
    string szString;
    szString = "</";
    szString.append(szName);
    szString.append(">\n");
	_ftprintf_s(m_pEvidenceFile, szString.c_str());
    MD5Update(&m_md5Data, (unsigned char *)szString.c_str(), szString.length());
    m_bNewLine = true;
}

//*************************************************************************************
//
//  Write
//
//*************************************************************************************
void HashedXMLFile::Write(const char *szValue)
{
    if (m_pEvidenceFile)
    {
	    _ftprintf_s(m_pEvidenceFile, szValue);
        MD5Update(&m_md5Data, (unsigned char *)szValue, strlen(szValue));
    }
    m_bNewLine = false;
}

//*************************************************************************************
//
//  WriteNodeWithValue
//
//*************************************************************************************
void HashedXMLFile::WriteNodeWithValue(char *szName, const char *szValue)
{
    string szString;

    szString = "<";
    szString.append(szName);
    szString.append(">");
    szString.append(szValue);
    szString.append("</");
    szString.append(szName);
    szString.append(">\n");
    if (m_pEvidenceFile)
    {
	    _ftprintf_s(m_pEvidenceFile, szString.c_str());
        MD5Update(&m_md5Data, (unsigned char *)szString.c_str(), szString.length());
    }
    m_bNewLine = false;
}

//*************************************************************************************
//
//  WriteAttribute
//
//*************************************************************************************
void HashedXMLFile::WriteAttribute(char *szName, const char *szValue)
{
    string szString;

    szString = " ";
    szString.append(szName);
    szString.append("=\"");
    szString.append(szValue);
    szString.append("\"");
    if (m_pEvidenceFile)
    {
	    _ftprintf_s(m_pEvidenceFile, szString.c_str());
        MD5Update(&m_md5Data, (unsigned char *)szString.c_str(), szString.length());
    }
    m_bNewLine = false;
}

//*************************************************************************************
//
//  Open
//
//*************************************************************************************
int HashedXMLFile::Open(char *szFilename)
{
    int iRetVal = 0;
    char *szString;

    // start a running hash
    MD5Init(&m_md5Data);

    // open the file
	_tfopen_s(&m_pEvidenceFile, szFilename, _T("w")); 
	if(m_pEvidenceFile)
	{
        szString = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
		_ftprintf_s(m_pEvidenceFile, szString);
        MD5Update(&m_md5Data, (unsigned char *)szString, strlen(szString));
    }
    else
    {
        // basic file io error
        iRetVal = -1;
    }

    return iRetVal;
}

//*************************************************************************************
//
//  Close
//
//*************************************************************************************
int HashedXMLFile::Close()
{
    unsigned char hash[20];
    // close the hash
    MD5Final(hash, &m_md5Data);

    // TBD:  Get the hash value 

    // close the file
    fclose(m_pEvidenceFile);

    return 0;
}

