
/*  ************************************************************************  *
 *				   fwmemmap.h				      *
 *  ************************************************************************  */

#pragma once

/*  This header file is for use in a kernel-mode driver and in a Win32
    application. Use nothing that isn't in both the SDK and WDK.  */

/*  ************************************************************************  */

/*  The FwMemMap driver exists to gather information in kernel mode. It is
    accessible from user mode under the following device and driver names.  */

#define FWMEMMAP_DEVICE_NAME		"FwMemMap"
#define FWMEMMAP_DRIVER_NAME		"FWMEMMAP.SYS"

/*  This header is for the first version.  */

#define MAKE_VERSION(major,minor)	(((ULONG) (major) << 0x10) | (minor))

#define FWMEMMAP_API_VERSION		MAKE_VERSION (1, 0)

/*  A user-mode application may "call" the driver through IOCTL functions.  */

#define FILE_DEVICE_FWMEMMAP		FILE_DEVICE_UNKNOWN

#define FWMEMMAP_CTL_CODE(func) \
    CTL_CODE (FILE_DEVICE_FWMEMMAP, (func), METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_FWMEMMAP_GET_VERSION	FWMEMMAP_CTL_CODE (0)
#define IOCTL_FWMEMMAP_GET_MAP		FWMEMMAP_CTL_CODE (1)

/*  Structures for the firmware map  */

#pragma pack (push, 8)			/*  in case driver and application
					    get compiled differently  */
typedef struct _E820_DESCRIPTOR {
    LARGE_INTEGER Base;
    LARGE_INTEGER Size;
    ULONG Type;
    ULONG ExtAttr;                      // not actually used by WINLOAD
} E820_DESCRIPTOR, *PE820_DESCRIPTOR;

typedef struct _E820_MAP {
    ULONG Count;
    E820_DESCRIPTOR Descriptors [1];
} E820_MAP, *PE820_MAP;

#pragma pack (pop)

/*  ************************************************************************  */

