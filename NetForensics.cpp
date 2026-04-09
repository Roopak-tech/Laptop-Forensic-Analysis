//This is a tester program that will use the functions exported by NetForensics.dll.
#ifdef NETFORENSICS

#define _UNICODE
#define UNICODE

#include "WSTRemote.H"

//extern WSTCommon commonResources;

typedef int (*NF)(WSTCommon &commonResources);

int WSTRemote::doNetForensics(WSTCommon &commonResources)
{
	
	HMODULE hModule = NULL;
	NF perfromNetForenscis = NULL;

	hModule = LoadLibrary(TEXT("NetForensics.dll"));

	if(hModule == NULL)
	{
		cout << "Module load failed :" << GetLastError() << endl;
		gui.DisplayError(_T("NetForensics.dll missing"));
		return ERR_MODULE_LOAD;
	}

	// Obtain exported handles
	perfromNetForenscis = (NF)GetProcAddress(hModule,"performNetForensics");

	if(perfromNetForenscis == NULL)
	{
		cout << "Could not obtain address of doNetForenscis function :" << GetLastError() << endl;
		gui.DisplayError(_T("NetForenscis extractor missing ..."));
		FreeLibrary(hModule);
		return ERR_FUNCTION_LOAD;
	}

	perfromNetForenscis(commonResources);

	FreeLibrary(hModule);
	return ERROR_SUCCESS;
}

#undef _UNICODE
#undef UNICODE

#endif NETFORENSICS