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

#ifndef DENTRY_HPP
#define DENTRY_HPP

#include <map>
#include <string>
#include <exception>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
//#include <iconv.h>

using namespace std;

namespace HotRodXML
{

  /**
   * Define the standard indent whitespace to use
   * when we serialize Dentry objects with human-
   * formatting output.
   */
  static string XMLIndentString = "  "; 
  /**
   * @addtogroup Core
   * @{
   * TODO: make dentry name tag indentification 
   * more robust.  It can be easily fooled in this
   * prototype version if one doesn't watch their 
   * tag names.
   */
  class Dentry : public map <string,string> {
    public:
      /**
       * Default Constructor. This method 
       * initializes 'myName' to "" and 
       * 'formatSerialization' to false.
       */
      Dentry();
     
      /**
       * Parametric Constructor. This method
       * intializes 'myName' to 'name' and 
       * 'formatSerialization' to format.
       */
      Dentry(const string &name, bool format);

      /**
       * Destructor. This method cleans up
       * the iconv context 'utf8Convert'.
       */
       ~Dentry();

      /**
       * Public Function. This method serializes
       * the Current Dentry object to an ostream&
       * object formatted as an XML element tree.
       * NOTE: Characters that are invalid in XML
       * text items are automatically remapped to their
       * delimited values (i.e. & becomes &amp;). 
       */
      void Serialize(ostream& os) ;

      /**
       * Public Effector. This method sets 'myName' 
       * to the value of 'newName'.
       */
      void setName(const string& newName);

      /**
       * Public Inspector. This method retrieves 
       * the value of 'myName'
       */
      const string& getName() const;
      bool Serialize(FILE* outFile);

      void changeFormat(bool whitespace);
      void fixXML(string& s) const;
      
      

  protected:
      
      /**
       *formatSerialization is a boolean flag that determines
       * whether to print human-friendly whitespace when 
       * serializing 'this'
       */
      bool formatSerialization;
      inline void indentDentry(int indentDepth) const;
      inline void indentDentry(FILE* outFile,int indentDepth) const;
      inline void indentDentry(ostream& os,int indentDepth) const;
      inline void printOpenTag(ostream& os,const string& tag) const;
      inline void printCloseTag(ostream& os,const string& tag) const;
      bool iconvCheck(string& s,size_t& startIndex) const;
      void forceUTF8(string& s) const;
      void dieNulls(string &s);
      void cleanSweep(string &s, string &hack);
    private:
      string myName;
      char* iconvBuffer;
  };
  /** @} */ //end to addtogroup Core

/**
 * Overload for stream extraction operator to 
 * serialize Dentry objects.
 */
ostream& operator<< (ostream& os, Dentry& d);


}

#endif
