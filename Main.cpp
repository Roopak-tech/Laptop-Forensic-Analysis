// Main.cpp : Contains the main function.
//

#include "stdafx.h"

//#include "WSTRemote.h"
//#include "SysData.h"
#include "WSTLog.h"


using namespace std;

extern "C" BOOL SelfDelete(BOOL fRemoveDirectory);

typedef enum {
	NONE, 
	PATH, 
	RECURSE, 
	PIPE, 
	OUTPUT, 
	GETPROCESSMEM,
	GETSCREENCAP
} ArgType;

int _tmain(int argc, _TCHAR* argv[])
{
//	WSTRemote *w = NULL;
	try {
//		w = new WSTRemote(TEXT("scanconfig.xml"));
//		if (w != NULL) {
			AppLog.Write("ScanConfig.xml successfully read", WSLMSG_STATUS);
			AppLog.Write("Starting Investigation", WSLMSG_STATUS);
			/*RJM*/
			
//			if(w->isLATT())
//			{
				AppLog.Write("Running US-LATT", WSLMSG_STATUS);
//				if(!w->SetupCaseDirectory())
//				{
//					AppLog.Write("Failed to create case directory");
//					exit(-4);
//				}
//			}
//			else
//				AppLog.Write("Running LiveWire", WSLMSG_STATUS);

			/*RJM*/
//			w->DoInvestigation();
//		}
	}
	catch (std::exception e) {
		tfstream f("wstfail.txt");
		f << "exception: " << e.what();
		f.close();
		AppLog.Write("Error creating WSTRemote", WSLMSG_DEBUG);
		AppLog.Write(e.what());
		return -1;
	}
	catch (...) {
		AppLog.Write("Generic error caught in Main.cpp", WSLMSG_DEBUG);
		return -2;
	}
	
//	delete w;
	//SelfDelete(TRUE);

	AppLog.Write("Exiting WSTRemote", WSLMSG_DEBUG);

	return 0;

}
