#include "StdAfx.h"

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include "DiskSubsys.h"
#include "windows.h"
#include "WSTLog.h"


DiskSubsys::DiskSubsys(void)
{
	EnumerateVolumes();
}

DiskSubsys::~DiskSubsys(void)
{
}


int DiskSubsys::EnumerateVolumes(void)
{
    return 0;
}