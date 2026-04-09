/******************************************************************************
* Filename..........: $URL$
* Original Author...: Matt Meno 
* Version...........: $Revision$
* Last Modified By..: $Author$
* Last Modified On..: $Date$
*
* Copyright 2005 I.D.E.A.L. Technology Corporation
*
* Please see LICENSE.TXT file for more information
******************************************************************************/

#include <map>
#include <string>
#include <sstream>
#include "Dentry.h"
#include <errno.h>
//#include <unistd.h>
#include <string.h>

using namespace std;
using namespace HotRodXML;

namespace HotRodXML
{

#define F 0   /* character never appears in text */
#define T 1   /* character appears in plain ASCII text */
#define I 2   /* character appears in ISO-8859 text */
#define X 3   /* character appears in non-ISO extended ASCII (Mac, IBM PC) */

const char text_chars[256] = {
  /*                  BEL BS HT LF    FF CR    */
  F, F, F, F, F, F, F, F, F, F, T, F, F, T, F, F,  /* 0x0X */
        /*                              ESC          */
  F, F, F, F, F, F, F, F, F, F, F, F, F, F, F, F,  /* 0x1X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x2X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x3X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x4X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x5X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, T,  /* 0x6X */
  T, T, T, T, T, T, T, T, T, T, T, T, T, T, T, F,  /* 0x7X */
  /*            NEL                            */
  X, X, X, X, X, T, X, X, X, X, X, X, X, X, X, X,  /* 0x8X */
  X, X, X, X, X, X, X, X, X, X, X, X, X, X, X, X,  /* 0x9X */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xaX */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xbX */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xcX */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xdX */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I,  /* 0xeX */
  I, I, I, I, I, I, I, I, I, I, I, I, I, I, I, I   /* 0xfX */
};

Dentry::Dentry()
{
  myName = "";
  formatSerialization = false;
}

Dentry::Dentry(const string &name, bool format)
: myName(name),formatSerialization(format)
{
}

Dentry::~Dentry()
{
}

void Dentry::Serialize(ostream& os) 
{
  utf8Convert = iconv_open("utf-8","utf-8");
  iconvBuffer = new char[BUFSIZ];

  //set our iterator to the beginning of the dentry
  Dentry::const_iterator i = begin();
  
  //print opening dentry tag
  printOpenTag(os,myName);
  if (formatSerialization)
  {
    os<<'\n';
  }
  
  //print member elements/text
  for (;i != end();i++){
    if (formatSerialization)
    {
      indentDentry(os,1);
    }
    printOpenTag(os,(*i).first);
    string s = (*i).second;
    fixXML(s);
    //make sure the string is utf-8
    forceUTF8(s);
    dieNulls(s);
    os<<s;
    printCloseTag(os,(*i).first);
    if (formatSerialization)
    {
      os<<'\n';
    }
  }
    
  //print closing dentry tag
  printCloseTag(os,myName);
  if (formatSerialization)
  {
    os<<'\n';
  }
  os.flush();
  
  iconv_close(utf8Convert);
  delete[] iconvBuffer;
  utf8Convert = 0;
}


void Dentry::changeFormat(bool whitespace)
{
  formatSerialization = whitespace;
}

void Dentry::setName(const string& newName)
{
  myName = newName;
}

const string& Dentry::getName() const
{
  return myName;
}

inline void Dentry::indentDentry(ostream& os,int indentDepth) const
{
  for (;indentDepth > 0;indentDepth--)
  {
    os<<XMLIndentString;
  }
}

inline void Dentry::indentDentry(FILE* outFile,int indentDepth) const
{
  for (;indentDepth > 0;indentDepth--)
  {
    fprintf(outFile,XMLIndentString.c_str());
  }
}

inline void Dentry::indentDentry(int indentDepth) const
{
  indentDentry(stdout,indentDepth);
}
inline void Dentry::printOpenTag(ostream& os,const string& tag) const
{
  //print tag opening
  os<<'<'<<tag<<'>';
}
inline void Dentry::printCloseTag(ostream& os,const string& tag) const
{
  //print tag closing
  os<<"</"<<tag<<'>';
}

bool Dentry::Serialize(FILE* outFile)
{
  //make sure outFile looks valid
  if (!outFile)
  {
    return false;
  }

  utf8Convert = iconv_open("utf-8","utf-8");
  iconvBuffer = new char[BUFSIZ];
  bool retVal = true; 
  //set our iterator to the beginning of the dentry
  Dentry::const_iterator i = begin();
  
  //print opening dentry tag
  if (retVal && fprintf(outFile,"<%s>",myName.c_str()) < 0)
  {
    retVal = false;
  }
  if (retVal && formatSerialization)
  {
    if (fprintf(outFile,"\n") < 0)
    {
      retVal = false;
    }
  }
  
  //print member elements/text
  for (;i != end();i++){
    if (!retVal)
    {
      break;
    }

    if (formatSerialization)
    {
      indentDentry(outFile,1);
    }
    string s = (*i).second; 
    fixXML(s);
    //make sure the string is utf-8
    forceUTF8(s);
    dieNulls(s);
    if (fprintf(outFile,"<%s>%s</%s>",(*i).first.c_str(),s.c_str(),(*i).first.c_str()) < 0)
    {
      retVal = false;
    }
    if (retVal && formatSerialization)
    {
      if (fprintf(outFile,"\n") < 0)
      {
        retVal = false;
      }
    }
  }
    
  //print closing dentry tag
  if (retVal && fprintf(outFile,"</%s>",myName.c_str()) < 0)
  {
    retVal = false;
  }

  if (retVal && formatSerialization)
  {
    if (fprintf(outFile,"\n") < 0)
    {
      retVal = false;
    }
  }

  if (retVal && fflush(outFile))
  {
    retVal = false;
  }
  iconv_close(utf8Convert);
  delete[] iconvBuffer;
  utf8Convert = 0;
 
  return retVal;  
}

bool Dentry::iconvCheck(string& s,size_t& startIndex) const
{
  /**
   * This function checks BUFSIZ bytes of 's' starting at (and including) 'startIndex'
   * for utf-8 conformance.  If an error is found, errorChar is set to the index of the
   * offending byte, false is returned.
   * Else, true is returned.
   */
  char* inCounter = (char*)(&(s.c_str()[startIndex]));
  char* outCounter = iconvBuffer;
  size_t inputSize = (s.size() - startIndex > BUFSIZ ? 
                      BUFSIZ : s.size()-startIndex);
  size_t outputSize = BUFSIZ;
  size_t startingInputSize = inputSize;
  bool retVal = true; 

  /*fprintf(stderr,"This is the start of a chunk\n");
  fprintf(stderr,"inputSize %d\n",inputSize);
  fprintf(stderr,"startIndex %d\n",startIndex);
  fprintf(stderr,"length %d\n",s.size());
  fprintf(stderr,"The start char of the sequence is %02x\n",(unsigned char)s[startIndex]);
  */

  //see if iconv() pukes on this buffer
  if (iconv(utf8Convert,
            &inCounter,
            &inputSize,
            &outCounter,
            &outputSize) == (size_t)(-1))
  {
    //check for an invalid sequence
    if (errno == EILSEQ)
    {
      //reset the state machine
      iconv(utf8Convert,NULL,NULL,NULL,NULL);
      
      //fprintf(stderr,"wow, I got an invalid sequence\n");
      startIndex += (startingInputSize - inputSize);
      retVal = false;
    }
  
    //check for a incomplete character at the end of the string
    else if (errno == EINVAL)
    {
      //reset the state machine
      iconv(utf8Convert,NULL,NULL,NULL,NULL);
     
      //fprintf(stderr,"Got an incomplete sequence starting at %d\n",startIndex + (startingInputSize - inputSize));
      
      //did we die in a place where we cannot get any more data (i.e. is this really an error?)
      if (s.size() - startIndex <= BUFSIZ)
      {
        //fprintf(stderr,"It is an error\n");
        retVal = false;
      } 
      /*else
      {
        fprintf(stderr,"It is NOT an error\n");
      }*/

      //set errorChar to the start of where we should process the next buffer
      startIndex += (startingInputSize - inputSize);
    } 
  }
 
  //if the buffer processed fine, just increment startIndex
  else
  {
    //fprintf(stderr,"Buffer processed fine\n");
    startIndex += startingInputSize;
  }

  //Is the state machine in a borked state?
  if (!retVal)
  {
  }

  return retVal;
}

void cleanSurrogates(string& s)
{
  char hexVal[7];
  size_t l = s.size();
  size_t i = 0;

  while (l > 2 && i < l-3)
  {
    if ((unsigned char)s[i] == 0xED &&
        (unsigned char)s[i+1] >= 0xA0 &&
        (unsigned char)s[i+1] <= 0xBF &&
        (unsigned char)s[i+2] >= 0x80 &&
        (unsigned char)s[i+2] <= 0xBF)
    {

      sprintf(hexVal,"[0x%02X]",(unsigned char)s[i+2]);
      s.insert(i+3,hexVal);
      sprintf(hexVal,"[0x%02X]",(unsigned char)s[i+1]);
      s.insert(i+3,hexVal);
      sprintf(hexVal,"[0x%02X]",(unsigned char)s[i]);
      s.insert(i+3,hexVal);
      s.erase(i,3);
      i+= 18;
      l = s.size();
    }
    else
      i++;
  }

}

void Dentry::cleanSweep(string &s, string &hack)
{
  size_t x = s.find(hack);
  while(x != string::npos)
  {
    s = s.erase(x,hack.size());
    x = s.find(hack);
  }
}

void Dentry::dieNulls(string &s)
{
  //HACK ALERT this is to sanitize the string so that expat does not barf
  //when sleuthkit does so
  //although vim intreprets this sequence weird it looks like this in hexedit
  // EF BC 80 EF  BF BF EF BF  BF EF BF BF  ks>.  <name>...........
  //the part that we are filtering here is the EF BF BF

  string hack = "￿";
  string hack2 = "￾";
  cleanSweep(s, hack);
  cleanSweep(s, hack2);
  cleanSurrogates(s);  
}

void Dentry::forceUTF8(string& s) const
{
  char hexVal[7];

  //use iconv to check 's' in BUFSIZ sized chunks
  size_t x = 0;
  while (x<s.size())
  {
    //check if iconv dies on a utf-8 -> utf-8 conversion
    if (!iconvCheck(s,x))
    {
      //replace the offending character with a "hex" value 
      sprintf(hexVal,"[0x%02X]",(unsigned char)s[x]);
      s.erase(x,1);
      s.insert(x,hexVal);
      x += 6;
    }

    //x is incremeted by iconvCheck
  }
}


void Dentry::fixXML(string& s) const
{
  int len = s.size();
  string tmp;
  char tripleDigit[10];
  
  //iterate and print the string
  for (int x = 0;x<len;x++)
  {
    switch (s[x]){
      case '&':
        tmp="&amp;";
        break;
      case '\'':
        tmp="&apos;";
        break;
      case '"':
        tmp="&quot;";
        break;
      case '>':
        tmp="&gt;";
        break;
      case '<':
        tmp="&lt;";
        break;
      default:
        tmp = "";
        
        break;
    }

    if (tmp.size())
    {
      s.erase(x,1);
      s.insert(x,tmp);
      x+= (tmp.size()-1);
      len = s.size();
    }
    //check if it is a control character
    else if (text_chars[(unsigned char)s[x]] == F)
    {
      sprintf(tripleDigit,"[0x%02X]",(unsigned char)s[x]);
      s.erase(x,1);
      s.insert(x,tripleDigit);
      x+= (strlen(tripleDigit)-1);
      len = s.size(); 
    }
  }
  
}

ostream& operator<< (ostream& os, Dentry& d)
{
  //use fast printf() workaround for stdout until entire 
  //backend is converted to straight-C.
  if (os == cout)
  {
    d.Serialize(stdout);
    return os;
  }
  d.Serialize(os);
  
  return os;
}

}


