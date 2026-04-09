// HashedXML.cpp : Defines the entry point for the console application.
//


#define _UNICODE
#define UNICODE

#include "winerror.h"
#include "stdafx.h"
#include "hashedxml.h"
#include "UTF8Validator/utf8.h"

extern string to_utf8(const wstring &);

extern string escapeXML(string);
string XMLDelimitString(string szSource);
string XMLDelimitString(const char *szValue);

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

    //_ftprintf_s(m_pEvidenceFile, "%s", szString.c_str());
	fprintf_s(m_pEvidenceFile, "%s", szString.c_str());
    fflush(m_pEvidenceFile);
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
	//_ftprintf_s(m_pEvidenceFile, "%s", szString.c_str());
	fprintf_s(m_pEvidenceFile, "%s", szString.c_str());
    fflush(m_pEvidenceFile);
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
//	_ftprintf_s(m_pEvidenceFile, "%s", szString.c_str());
	fprintf_s(m_pEvidenceFile, "%s", szString.c_str());
    fflush(m_pEvidenceFile);
    MD5Update(&m_md5Data, (unsigned char *)szString.c_str(), szString.length());
    m_bNewLine = true;
}

//*************************************************************************************
//
//  Write
//
//*************************************************************************************
void HashedXMLFile::Write(const wchar_t *szValue) {
	wstring wValue(szValue);
	Write(to_utf8(wValue).c_str());
}

void HashedXMLFile::Write(const char *szValue)
{
	string escapedString=szValue;
	size_t index;

	
	char findStr[]={'&','\0'};
	if((index=escapedString.find_first_of(findStr,0))!=escapedString.npos)
	{
		string seq=escapedString.substr(index+1,index+3);
		string c=escapedString.substr(index);
		if(strncmp(seq.c_str(),"amp",3)&&strncmp(seq.c_str(),"quo",3)&&strncmp(seq.c_str(),"apo",3)&&strncmp(seq.c_str(),"lt",2)&&strncmp(seq.c_str(),"gt",2))
			escapedString = escapedString.substr(0,index)+"&amp;"+escapedString.substr(index+1);
			//cout<<szValue<<endl;
	}
	
    if (m_pEvidenceFile)
    {
		string temp;
		utf8::replace_invalid(escapedString.begin(),escapedString.end(), back_inserter(temp),'!');
		escapedString=temp;
		fprintf_s(m_pEvidenceFile, "%s", escapedString.c_str());
        fflush(m_pEvidenceFile);
		MD5Update(&m_md5Data, (unsigned char *)escapedString.c_str(), strlen(szValue));
    }
    m_bNewLine = false;
}

//*************************************************************************************
//
//  WriteNodeWithValue
//
//*************************************************************************************
void HashedXMLFile::WriteNodeWithValue(char *szName,const wchar_t *szValue)
{
	wstring wValue;
	wValue=szValue;
	WriteNodeWithValue(szName, to_utf8(wValue).c_str());
}

void HashedXMLFile::WriteNodeWithValue(char *szName, const char *szValue)
{
    string szString;
    string szXMLValue;

    szXMLValue = XMLDelimitString(szValue);

    szString = "<";
    szString.append(szName);
    szString.append(">");
    szString.append(szXMLValue);
    szString.append("</");
    szString.append(szName);
    szString.append(">\n");
	
    if (m_pEvidenceFile)
    {
//	    _ftprintf_s(m_pEvidenceFile, "%s", szString.c_str());
		string temp;
		utf8::replace_invalid(szString.begin(), szString.end(), back_inserter(temp),'!');
		szString=temp;
		fprintf_s(m_pEvidenceFile, "%s", szString.c_str());
        fflush(m_pEvidenceFile);
        MD5Update(&m_md5Data, (unsigned char *)szString.c_str(), szString.length());
    }
    m_bNewLine = false;
}

//*************************************************************************************
//
//  WriteAttribute
//
//*************************************************************************************
void HashedXMLFile::WriteAttribute(char *szName, const wchar_t *szValue)
{
	wstring wValue(szValue);
	WriteAttribute(szName, to_utf8(wValue).c_str());
}

void HashedXMLFile::WriteAttribute(char *szName, const char *szValue)
{
    string szString;
    string szXMLValue;

    szXMLValue = XMLDelimitString(szValue);

    szString = " ";
    szString.append(szName);
    szString.append("=\"");
    szString.append(szXMLValue);
    szString.append("\"");
    if (m_pEvidenceFile)
    {
//	    _ftprintf_s(m_pEvidenceFile, "%s", szString.c_str());
		fprintf_s(m_pEvidenceFile, "%s", szString.c_str());
        fflush(m_pEvidenceFile);
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

    //m_szXMLFilename = szFilename;

    // open the file
	fopen_s(&m_pEvidenceFile, szFilename, "w"); 
	//m_pEvidenceFile = fopen(szFilename,"w, ccs=UTF-8");
	

	if(m_pEvidenceFile)
	{
        szString = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n";
//		_ftprintf_s(m_pEvidenceFile, "%s", szString);
		fprintf_s(m_pEvidenceFile, "%s", szString);
		
		//fprintf(m_pEvidenceFile,"%s",szString);
        fflush(m_pEvidenceFile);
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
int HashedXMLFile::Close(unsigned char *hash)
{
    // close the hash
    if (hash)
       MD5Final(hash, &m_md5Data);

    // close the file
    fclose(m_pEvidenceFile);

    return 0;
}


#undef UNICODE
#undef _UNICODE