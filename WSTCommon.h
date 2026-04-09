#pragma once


#ifndef _WSTCOMMON_H_
#define _WSTCOMMON_H_

//#define WINDOWS_LEAN_AND_MEAN

#define ERR_FUNCTION_LOAD -1001
#define ERR_MODULE_LOAD -1002

#include "gui.h"		// gui_resources
#include "Logger.h"		// WSTLog
#include "hashedxml.h"	// HashedXMLFile
#include <Windows.h>	// OSVERSIONINFOEX
#include "defs.h"		// ScanConfig

// Common structure used by US-LATT modules
struct WSTCommon
{
	~WSTCommon()
	{
		cout << "~WSTCommon" <<  endl;
	}

	gui_resources gui;
	WSTLog AppLog;
	HashedXMLFile repository;
	OSVERSIONINFOEX osvex;
	ScanConfig config;
};


#endif _WSTCOMMON_H_