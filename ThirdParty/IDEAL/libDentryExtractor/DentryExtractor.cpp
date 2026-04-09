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

#include "DentryExtractor.h"

using namespace HotRodXML;

namespace HotRodXML
{

  DentryExtractor::DentryExtractor()
  {
    depth = 0;
  }
  
  DentryExtractor::~DentryExtractor()
  {
  }
  
  DentryExtractor::DentryExtractor(bool formatting)
  :dentryBuffer("",formatting), formatOutput(formatting)
  {
    depth = 0;
  }

  void DentryExtractor::changeFormat(bool whiteSpace)
  {
    dentryBuffer.changeFormat(whiteSpace);
    formatOutput = whiteSpace;
  }

  void DentryExtractor::streamBegin()
  {
    //block SIGUSR1 for polling
//    toggleSignal(SIGUSR1,0);
  
    cout<<"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" 
        <<"<root>\n";
    cout.flush(); 

  }

  void DentryExtractor::beginXMLStream()
  {
    streamBegin();
  }

  void DentryExtractor::streamEnd()
  {
    cout<<"</root>\n";
    cout.flush();

    //unblock SIGUSR1
//    toggleSignal(SIGUSR1,1);
  }

  void DentryExtractor::endXMLStream()
  {
    streamEnd();
  }

  void DentryExtractor::startOfElement(const string &name)
  {
    //increase our recorded depth
    depth++;

    //dentry opening/closing tags are at depth 2
    if (depth == 2)
    {
      //clear the dentryBuffer and rename it
      dentryBuffer.clear(); 
      dentryBuffer.setName(name);
    }
    //dentry elements are at depth 3
    else if (depth == 3)
    {
      //start filling the elementBuffer
      elementBuffer.first = name;
      elementBuffer.second = "";
    }
    //allow for arbitrary XML inclusion, just treat as string data
    else if (depth > 3)
    {
      elementBuffer.second += "<";
      elementBuffer.second += name;
      elementBuffer.second += ">";
    }
  }

  
  void DentryExtractor::endOfElement(const string &name)
  {

    //check if this is the end of the dentry or just
    //another element
    if (depth == 3)
    {
      dentryBuffer.insert(elementBuffer);
    }
    //check if this closes a dentry
    else if (depth == 2)
    {
      //call the dentryExtracted callback
      preDentryExtracted(dentryBuffer);
    }
    //allow for arbitrary XML inclusion, just treat as string data
    else if (depth > 3)
    {
      elementBuffer.second += "</";
      elementBuffer.second += name; 
      elementBuffer.second += ">";
    }

    //decrement our depth
    depth--;
  }
  
  void DentryExtractor::handleText(const string &data)
  {
    elementBuffer.second += data;
  }
  
  void DentryExtractor::preDentryExtracted(Dentry& newDentry)
  {
/*    if (!signalPending(SIGUSR1))
    {
      dentryExtracted(newDentry);
    }
    else
    {*/
      cout<<newDentry;
    //}
  }

  /*
  void DentryExtractor::toggleSignal(int sig,bool handle)
  {
    //block signal
    sigset_t signalsToBlock;
    sigemptyset(&signalsToBlock);
    sigaddset(&signalsToBlock,sig);
    sigprocmask(
        (handle ? SIG_UNBLOCK : SIG_BLOCK)
        , &signalsToBlock,NULL);

    //ignore signals for when they are cleared
    signal(sig,(handle ? SIG_DFL : SIG_IGN));
  }
  */

  bool DentryExtractor::signalPending(int sig)
  {
    //check our signals
    sigset_t signals;
    sigpending(&signals);
    return sigismember(&signals,sig); 
  }

  void DentryExtractor::clearSignal(int sig)
  {
    //create the set of signals to unblock
    sigset_t signalsToBlock;
    sigemptyset(&signalsToBlock);
    sigaddset(&signalsToBlock,sig);

    //unblock/clear the signal
    sigprocmask(SIG_UNBLOCK,&signalsToBlock,NULL);

    //reblock the signal
    sigprocmask(SIG_BLOCK,&signalsToBlock,NULL);
  }

}
