#include "stdafx.h"
#include "DisplayInfo.h"

DisplayInfo::DisplayInfo()
{
	int DisplayNum = 0;
	DEVMODE dm;
	DISPLAY_DEVICE dd;
	DISPLAYINFO di;

	memset(&dd, 0, sizeof(DISPLAY_DEVICE));
	dd.cb = sizeof(DISPLAY_DEVICE);

	while(EnumDisplayDevices(NULL, DisplayNum++, &dd, EDD_GET_DEVICE_INTERFACE_NAME))
	{
		if((dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) & !(dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER))
		{
			// Get the display adapter info
			memset(&dm, 0, sizeof(DEVMODE));
			dm.dmSize = sizeof(DEVMODE);
			dm.dmSpecVersion = DM_SPECVERSION;

			EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm);

//			_tcscpy(di.DeviceID, dd.DeviceID);
			_tcscpy_s(di.DeviceID, sizeof(di.DeviceID), dd.DeviceID);
//			_tcscpy(di.DeviceName, dd.DeviceName);
			_tcscpy_s(di.DeviceName, sizeof(di.DeviceName), dd.DeviceName);
//			_tcscpy(di.DeviceString, dd.DeviceString);
			_tcscpy_s(di.DeviceString, sizeof(di.DeviceString), dd.DeviceString);
			strcpy_s(di.DriverName, sizeof(di.DriverName), (char*)dm.dmDeviceName);
			di.BitsPerPixel = dm.dmBitsPerPel;
			di.PixelsX = dm.dmPelsWidth;
			di.PixelsY = dm.dmPelsHeight;
			di.RefreshRate = dm.dmDisplayFrequency;

			vecDisplays.push_back(di);
		}

		memset(&dd, 0, sizeof(DISPLAY_DEVICE));
		dd.cb = sizeof(DISPLAY_DEVICE);
	}
}