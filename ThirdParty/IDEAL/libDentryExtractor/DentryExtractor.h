/******************************************************************************
* Filename..........: $URL$
* Original Author...: Matt Meno 
* Version...........: $Revision$
* Last Modified By..: $Author$
* Last Modified On..: $Date$
*
* Copyright 2006 I.D.E.A.L. Technology Corporation
*
* Please see LICENSE.TXT file for more information
******************************************************************************/

#ifndef DENTRYEXTRACTOR_HPP
#define DENTRYEXTRACTOR_HPP

#include <exception>
#include <signal.h>

//#include <PipelineExpatParser.hpp>
#include "Dentry.h"

using namespace std;
using namespace HotRodXML;

namespace HotRodXML
{
  /**
   * This is a kludge to identify the tags of Dentry elements in an XML
   * stream.
   * TODO: Make this more elegant/bug-proof.
   */
  static string DENTRY_ELEMENT_NAMES = "directory entry"; 

  /**
   * @addtogroup Core
   * @{
   * DentryExtractor is windowed PipelineExpatParser that
   * performs caching of elements in an XML stream
   * known as 'Dentry's (see Dentry.hpp). Subclasses must 
   * implement a single callback named dentryExtracted()
   * that will be called when a Dentry is encountered
   * on the input stream.
   */
  class DentryExtractor : public PipelineExpatParser
  {
    public:
      /**
       * Default Constructor. 
       */
      DentryExtractor();

      /**
       * Parametric Constructor. This method initializes
       * 'formatOutput' to 'formatting'.  This has the
       * implication of all Dentry objects instanciated
       * by 'this' having 'foratting' as their flag for
       * printing human-readable output (i.e. with 
       * whitespace).
       */
      DentryExtractor(bool formatting);

      virtual ~DentryExtractor();

      /**
       * Virtual Public Effector. This method updates
       * 'formatOutput' and 'dentryBuffer' to serialize
       * whitespace based on the value of 'formatting'
       */
      virtual void changeFormat(bool whiteSpace);

    protected:
      /**
       * Protected Virtual Callback Function.
       * streamBegin() prints the XML header and opening
       * <root> tag to the output stream so we generate
       * valid XML output.
       * TODO: Make more elegant in future.
       */
      virtual void streamBegin();

      /**
       * Protected Virtual Callback Function. This 
       * method wraps streamBegin() to allow other
       * libraries (namely DentryParser) to remap
       * streamBegin() and streamEnd() behavior 
       * yet offer a consistent programming interface.
       */
      virtual void beginXMLStream();

      /**
       * Protected Virtual Callback Function.
       * streamEnd() prints the closing <root> tag on the 
       * output stream so we have a valid XML stream at the
       * end of targeting.
       * TODO: More elegant in future?
       */
      virtual void streamEnd();

      /**
       * Protected Virtual Callback Function. This 
       * method wraps streamBegin() to allow other
       * libraries (namely DentryParser) to remap
       * streamBegin() and streamEnd() behavior 
       * yet offer a consistent programming interface.
       */
      virtual void endXMLStream();

      /**
       * Protected Callback Function. startOfElement()
       * checks 'name' against the known dentry-starting
       * tags and either starts caching a new Dentry
       * in dentryBuffer or caches 'name' as a new 
       * element in the current Dentry cached in 
       * 'dentryBuffer'.
       */
      void startOfElement(const string &name);

      /**
       * Protects Callback Function. endOfElement()
       * checks 'name' against the known dentry tags 
       * to determine if we are done parsing the current
       * dentry or if we are finished parsing an element
       * for the current dentry and consequently need
       * to insert 'elementBuffer' into 'dentryBuffer'
       */
      void endOfElement(const string &name);

      /**
       * Protected Callback Function. handleText()
       * adds 'data' to the elementBuffer.
       */
      void handleText(const string &data);
      
      /**
       * Protected Pure-Virtual Callback Function.
       * This method MUST be defined by subclasses 
       * and will be called from the underlying 
       * XML parsing engine when a dentry has been
       * encountered and cached from the XML input
       * stream.
       */
      virtual void dentryExtracted(Dentry& newDentry) = 0;

      /**
       * Protected Callback Function. This method
       * is a wrapper for dentryExtracted() that
       * performs a shortcut passthrough of all
       * dentries to stdout upon receiving 
       * a SIGUSR1 signal.
       */
      void preDentryExtracted(Dentry& newDentry);
      
      /**
       * Protected Function. This method toggles
       * blocking/unblocking of signal 'sig' to the current
       * process. If 'handle' is true, the signal is 
       * unblocked and the handler is set to be the 
       * default action. Else, handle is blocked and 
       * set to be ignored.
       */
      void toggleSignal(int sig,bool handle);

      /**
       * Protected Function. This method returns
       * whether signal 'sig' is pending for 
       * delivery to the current process.
       */
      bool signalPending(int sig);
      
      /**
       * Protected Function. This method clears
       * a pending 'sig' signal to the current
       * process by unblocking and re-blocking 
       * the signal.
       */
      void clearSignal(int sig);

      /** 
       * 'formatOutput' determines whether the Dentry 
       * objects instanciated by 'this' will serialize
       * with human-friendly whitespace (true) or as
       * pure minimalistic XML data (false).
       */
      bool formatOutput;

      /** Stores the currently caching/cached Dentry.*/
      Dentry dentryBuffer;
      
      /** Stores the currently caching/cached element.*/
      pair <string,string> elementBuffer;

      /** Tracks the our depth in the dentry stream. */
      size_t depth;
  };
  /** @} */ //end of addtogroup Core

}

#endif
