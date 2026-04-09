#define _UNICODE
#define UNICODE

#pragma once

#include "iphlpapi.h"
#include "HashedXML.h"
#include <vector>

using namespace std;

typedef DWORD (WINAPI *lpfnAllocateAndGetTcpExTableFromStack)(PVOID *ppTcpExTable, BOOL bOrder, HANDLE hHeap, DWORD dwFlags, DWORD dwFamily);
typedef DWORD (WINAPI *lpfnAllocateAndGetUdpExTableFromStack)(PVOID *PPUdpExTable, BOOL bOrder, HANDLE hHeap, DWORD dwFlags, DWORD dwFamily);
typedef DWORD (WINAPI *lpfnGetExtendedTcpTable)(PVOID pTcpTable, PDWORD pdwSize, BOOL bOrder, ULONG ulAf, TCP_TABLE_CLASS TableClass, ULONG Reserved);
typedef DWORD (WINAPI *lpfnGetExtendedUdpTable)(PVOID pTcpTable, PDWORD pdwSize, BOOL bOrder, ULONG ulAf, UDP_TABLE_CLASS TableClass, ULONG Reserved);

typedef enum _ENUM_Protocol {
	PROTO_TCP = 0,
	PROTO_UDP
} PROTO;

typedef struct _STRUCT_NetworkInterface {
	DWORD dwIfIndex;
	DWORD dwIfType;
	BOOL  bEnabled;
	DWORD dwStatus;
	TCHAR szIPv4Addr[16];	// ###.###.###.###
	TCHAR szGateway[16];	// ###.###.###.###
	TCHAR szSubnet[16];		// ###.###.###.###
	TCHAR szMACAddr[24];	// XX-XX-XX-XX-XX-XX
} NetworkInterface, *PNetworkInterface;

typedef struct _STRUCT_PortInfo {
	PROTO dwType;
	TCHAR szLocalIPv4[16];
	DWORD dwLocalPort;
	TCHAR szRemoteIPv4[16];
	DWORD dwRemotePort;
	DWORD dwState;
	DWORD dwOwnerPID;
} PortInfo, *PPortInfo;

class NetworkInfo
{
public:
	NetworkInfo(void);
	~NetworkInfo(void);

	int GetPortsAndPIDS(void);
	int GetRoutingTable(void);
	int GetARPTable(void);
	int GetMACAddress(void);
	int GetAdapterConfig(void);
	int EnumerateAdapters(void);
	int CapturePackets(LPCTSTR szFileName, int nSeconds);
	int WriteXML(HashedXMLFile &repository);

private:
	vector<NetworkInterface> vecInterfaces;
	vector<PortInfo> vecOpenPorts;
	TCHAR szNetBIOSName[MAX_PATH];
	TCHAR szHostName[MAX_PATH];
};

