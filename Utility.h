#pragma once
#include "stdafx.h"

using namespace std;


class UnifiedLog
{

public:

	UnifiedLog(void);
	~UnifiedLog(void);

	void LogMessage(tstring message);
	void WriteToFile(tstring filename);

private:

	tstringstream internalLog;
	tfstream output;

};
