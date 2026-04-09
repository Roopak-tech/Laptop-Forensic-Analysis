#include "Utility.h"

UnifiedLog::UnifiedLog()
{}

UnifiedLog::~UnifiedLog()
{}

void UnifiedLog::LogMessage(tstring message)
{

	SYSTEMTIME time;
	tstring msg;
	GetLocalTime(&time);
	
	msg.assign(message);
	internalLog.fill('0');
	internalLog 
		<< setw(2) << time.wMonth << TEXT("-") 
		<< setw(2) << time.wDay << TEXT("-") 
		<< time.wYear << TEXT(" ")
		<< setw(2) << time.wHour << TEXT(":") 
		<< setw(2) << time.wMinute << TEXT(":") 
		<< setw(2) << time.wSecond << TEXT(" ")
		<< message << endl;

}

void UnifiedLog::WriteToFile(tstring filename) {

	output.open(filename.c_str(), ios::out);
	output << internalLog.str();
	output.close();

}