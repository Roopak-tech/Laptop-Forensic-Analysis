// DiskSubsys.h -
// Starting point to enumerate the disk subsystems
// 1/30/08 - Initial version

#pragma once

#ifndef _DISKSUBSYS_H_
#define _DISKSUBSYS_H_

#include "windows.h"
#include "tchar.h"

#include "DiskVolume.h"
#include "PhysicalDrive.h"

#include <vector>

using namespace std;

class DiskSubsys
{
public:
	DiskSubsys(void);
	~DiskSubsys(void);

public:
	// Definitely want to create a "map" function that spits out the layout
	// THAT would be cool.

	int EnumerateVolumes(void);
	int EnumerateDrives(void);

public:
	vector<DiskVolume>	  Volumes;		// Array of volumes
};

#endif // _DISKSUBSYS_H_