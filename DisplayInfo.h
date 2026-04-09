#pragma once

#include "stdafx.h"
#include <vector>

using namespace std;

typedef struct _STRUCT_DisplayInfo {
	int PixelsX;
	int PixelsY;
	int BitsPerPixel;
	int RefreshRate;
	TCHAR DeviceName[CCHDEVICENAME];
	TCHAR DeviceID[128];
	TCHAR DeviceString[128];
	char DriverName[32];
} DISPLAYINFO, *PDISPLAYINFO;

class DisplayInfo {
public:
	DisplayInfo(void);

public:
	vector<DISPLAYINFO> vecDisplays;
};