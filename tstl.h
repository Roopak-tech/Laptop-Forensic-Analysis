// tstl.h - header file for TCHAR equivalents of STL
//          string and stream classes
//
// Copyright (c) 2006 PJ Arends
//
// This file is provided "AS-IS". Use and/or abuse it
// in any way you feel fit.
// 
// 2/15/07 DJS	Added defines for _tfindfirst
 
#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

 
namespace std
{
#if defined UNICODE || defined _UNICODE
 
    typedef wstring         tstring;
 
    typedef wstringbuf      tstringbuf;
    typedef wstringstream   tstringstream;
    typedef wostringstream  tostringstream;
    typedef wistringstream  tistringstream;
 
    typedef wstreambuf      tstreambuf;
 
    typedef wistream        tistream;
    typedef wiostream       tiostream;
 
    typedef wostream        tostream;
 
    typedef wfilebuf        tfilebuf;
    typedef wfstream        tfstream;
    typedef wifstream       tifstream;
    typedef wofstream       tofstream;
 
    typedef wios            tios;
 
#   define tcerr            wcerr
#   define tcin             wcin
#   define tclog            wclog
#   define tcout            wcout

#	define _tfindfirst		_wfindfirst
#	define _tfindnext		_wfindnext
#	define _tfinddata_t		_wfinddata_t
#	define _tstrstr			wcsstr
#	define _tatoi			_wtoi
#	define _tatol			_wtol
#	define _tstrchr			wcschr
#	define _tmemset			wmemset
#else // defined UNICODE || defined _UNICODE
 
    typedef string          tstring;
 
    typedef stringbuf       tstringbuf;
    typedef stringstream    tstringstream;
    typedef ostringstream   tostringstream;
    typedef istringstream   tistringstream;
 
    typedef streambuf       tstreambuf;
 
    typedef istream         tistream;
    typedef iostream        tiostream;
 
    typedef ostream         tostream;
 
    typedef filebuf         tfilebuf;
    typedef fstream         tfstream;
    typedef ifstream        tifstream;
    typedef ofstream        tofstream;
 
    typedef ios             tios;
 
#   define tcerr            cerr
#   define tcin             cin
#   define tclog            clog
#   define tcout            cout

#	define _tfindfirst		_findfirst
#	define _tfindnext		_findnext
#	define _tfinddata_t		_finddata_t
#	define _tstrstr			strstr
#	define _tatoi			atoi
#	define _tatol			atol
#	define _tstrchr			strchr
#	define _tmemset			memset

#endif // defined UNICODE || defined _UNICODE
} // namespace std