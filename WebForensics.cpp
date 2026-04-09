
//This is a tester program that will use the functions exported by WebForensics.dll.
#ifdef WEBFORENSICS

#define _UNICODE
#define UNICODE


#include "WebForensics.h"
#include "WSTRemote.H"
//#include "WSTLog.h"

const char *histQuery = "SELECT datetime(moz_historyvisits.visit_date/1000000,'unixepoch'), moz_places.url,moz_places.visit_count,moz_places.title,moz_historyvisits.visit_type FROM moz_places, moz_historyvisits WHERE moz_places.id = moz_historyvisits.place_id ORDER BY moz_historyvisits.visit_date DESC";


int WSTRemote::ExtractWebActivity(WSTLog &AppLog)
{
	HMODULE hModule=NULL;
	PFMPF FindMozillaPlacesDBFile=NULL; 
	OXML OpenWebForensicsXML=NULL;
	CXML CloseWebForensicsXML=NULL;
	QSDB QuerySqliteDb=NULL;
	GIFL GetIndexFileLocations=NULL;
	PIDF ParseIndexDotDatFile=NULL;
	PSF PerformSkypeForensics=NULL;
	FILE *fd=NULL;
	int moduleRet=ERROR_SUCCESS;

	hModule = LoadLibrary(TEXT("WebForensics.dll"));

	if(hModule == NULL)
	{
		cout<<"Module load failed :"<<GetLastError()<<endl;
		gui.DisplayError(_T("WebForensics.dll missing"));
		return ERR_MODULE_LOAD;
	}


	//Obtain all the exported handles 
	FindMozillaPlacesDBFile = (PFMPF)GetProcAddress(hModule,"FindMozillaPlacesDBFile");

	if(FindMozillaPlacesDBFile == NULL)
	{
		cout<<"Could not obtain address of FindMozillaPlacesDBFile function :"<<GetLastError()<<endl;
		gui.DisplayError(_T("Firefox extractor missing ..."));
		FreeLibrary(hModule);
		return ERR_FUNCTION_LOAD;
	}

	OpenWebForensicsXML = (OXML)GetProcAddress(hModule,"OpenWebForensicsXML");

	if(OpenWebForensicsXML == NULL)
	{
		cout<<"Could not obtain address of OpenWebForensicsXML function :"<<GetLastError()<<endl;
		FreeLibrary(hModule);
		return ERR_FUNCTION_LOAD;
	}

	CloseWebForensicsXML = (CXML)GetProcAddress(hModule,"CloseWebForensicsXML");

	if(CloseWebForensicsXML == NULL)
	{
		cout<<"Could not obtain address of CloseWebForensicsXML function :"<<GetLastError()<<endl;
		FreeLibrary(hModule);
		return ERR_FUNCTION_LOAD;
	}

	QuerySqliteDb = (QSDB)GetProcAddress(hModule,"QuerySqliteDb");

	if(QuerySqliteDb == NULL)
	{
		cout<<"Could not obtain address of QuerySqliteDb function :"<<GetLastError()<<endl;
		FreeLibrary(hModule);
		return ERR_FUNCTION_LOAD;
	}

	TCHAR ** placesLocation=NULL;
	int count;
	
	//Open the WebForensics xml file
	OpenWebForensicsXML(fd,TEXT("Files\\Web\\WebForensics_incomplete.xml"));

	if(fd == NULL)
	{
		gui.DisplayWarning(_T("Could not obtain web history. Open XML failed."));
		return ERR_FILE_OPEN;
	}

	//Find all the file locations for firefox places.sqlite file
	gui.DisplayStatus(_T("Collecting Firefox web activity ..."));


	moduleRet = FindMozillaPlacesDBFile(placesLocation,count);
	
	if(moduleRet != ERROR_SUCCESS)
	{
		gui.DisplayStatus(_T("Unable to access Firefox history."));
	}
	else if(count == 0)
	{
		gui.DisplayStatus(_T("No Firefox history found."));
	}

	cout<<"Firefox Profile locations:"<<endl;
	for(int i=0;i<count;i++)
	{
		cout<<placesLocation[i]<<endl;
		//Pass each profile file name to QSDB
		QuerySqliteDb(placesLocation[i],histQuery,fd);
	}

	FREE_PLACE_LOCATIONS //free placeLocation array (Firefox related)
	fclose(fd);

	//Open the web xml again
	OpenWebForensicsXML(fd,TEXT("Files\\Web\\WebForensics_incomplete.xml"));
	//Collect IE data
	GetIndexFileLocations = (GIFL)GetProcAddress(hModule,"GetIndexFileLocations");

	if(GetIndexFileLocations == NULL)
	{
		
		cout<<"Could not obtain address of GetIndexFileLocations function :"<<GetLastError()<<endl;
		FreeLibrary(hModule);
		CloseWebForensicsXML(fd,TEXT("Files\\Web\\WebForensics_incomplete.xml"));
		return ERR_FUNCTION_LOAD;
	}

	ParseIndexDotDatFile = (PIDF)GetProcAddress(hModule,"ParseIndexDotDatFile");

	if(ParseIndexDotDatFile == NULL)
	{
		cout<<"Could not obtain address of ParseIndexDotDatFile function :"<<GetLastError()<<endl;
		FreeLibrary(hModule);
		CloseWebForensicsXML(fd,TEXT("Files\\Web\\WebForensics_incomplete.xml"));
		return ERR_FUNCTION_LOAD;
	}

	TCHAR indexFileLocation[MAX_PATH+1];
	memset(indexFileLocation,0,MAX_PATH+1);

	gui.DisplayStatus(_T("Collecting Internet Explorer web activity ..."));

	if( (moduleRet = GetIndexFileLocations(indexFileLocation)) != ERROR_SUCCESS)
	{
		gui.DisplayStatus(_T("No Internet Explorer history found."));
		cout<<"Index File not found"<<endl;
		//CloseWebForensicsXML(fd,TEXT("Files\\Web\\WebForensics_incomplete.xml"));
	}
	else
	{
		cout<<"File Location :"<<indexFileLocation<<endl;
	}

	ParseIndexDotDatFile(indexFileLocation,fd);//Parse the Index.dat file
	
	//Close WebForensics xml file
	CloseWebForensicsXML(fd,TEXT("Files\\Web\\WebForensics_incomplete.xml"));


	//Collect SkypeForensics Data
	gui.DisplayStatus(_T("Collecting Skype forensic data ..."));
	gui.DisplayState(_T("Running Skype Forensics module"));
	gui.DisplayResult(_T("Acquiring Skype forensics data ..."));

	PerformSkypeForensics = (PSF)GetProcAddress(hModule,"PerformSkypeForensics");

	if(PerformSkypeForensics == NULL)
	{
		cout<<"Could not obtain address of PerformSkypeForensics function :"<<GetLastError()<<endl;
		FreeLibrary(hModule);
		return ERR_FUNCTION_LOAD;
	}


	if( (moduleRet = PerformSkypeForensics(AppLog, gui)) != ERROR_SUCCESS)
	{
		//cout<<"Skype Forensics Failed"<<endl;
		gui.DisplayError(_T("Error while collecting Skype forensic data ..."));
		FreeLibrary(hModule);
		return moduleRet;
	}
	gui.DisplayResult(_T("Skype Forensics Complete"));
	
	FreeLibrary(hModule);

	gui.DisplayStatus(_T("Web activity collection complete."));
	
	return ERROR_SUCCESS;

}

#undef _UNICODE
#undef UNICODE
#endif