
#include <vector>
#include <string>
#include <map>
#include "WSTRemote.h"

using namespace std;
	typedef struct searchParams
	{
		vector<wstring> drives;
		map<wstring,int> extensions;
		wstring lattDrive;
		unsigned long long maxFileSize;
		int previousDays;
		int createdDate;
		int modifiedDate;


	}searchParams;

//	extern int searchMultipleTargets(searchParams sp,WSTRemote *);

//namespace FileSearch
//{
//	__declspec(dllexport) int searchMultipleTargets(searchParams sp,WSTRemote *);
//__declspec(dllexport) int searchMultipleTargets(searchParams sp);
//}

