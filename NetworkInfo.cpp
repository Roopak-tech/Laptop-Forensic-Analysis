#define _UNICODE
#define UNICODE

#include "stdafx.h"

#include "windows.h"
#include "tchar.h"
#include "iphlpapi.h"
#include "NetworkInfo.h"
#include "xmlParser.h"
#include "WSTLog.h"
#include "SysCallsLog.h"
#pragma comment(lib, "ws2_32.lib")

extern OSVERSIONINFOEX g_osvex;

NetworkInfo::NetworkInfo(void)
{
	WSADATA wsData;

	memset(&wsData, 0, sizeof(WSADATA));
	INC_SYS_CALL_COUNT(WSAStartup); // Needed to be INC TKH
	WSAStartup(MAKEWORD(1, 0), &wsData);
}

NetworkInfo::~NetworkInfo(void)
{
	INC_SYS_CALL_COUNT(WSACleanup); // Needed to be INC TKH
	WSACleanup();
}

int NetworkInfo::GetPortsAndPIDS(void)
{
	DWORD dwSizeOfTable = 0;
	DWORD dwRetVal = 0;
	PVOID tcpextable = NULL;
	PVOID udpextable = NULL;
	MIB_TCPTABLE_OWNER_PID *pTCPTableOwnerPID = NULL;
	MIB_UDPTABLE_OWNER_PID *pUDPTableOwnerPID = NULL;
	in_addr *pin_addr = NULL;
	PortInfo pi;
    lpfnGetExtendedTcpTable MyGetExtendedTcpTable = NULL;
    lpfnGetExtendedUdpTable MyGetExtendedUdpTable = NULL;

	int len=0;
	wchar_t *waddr=NULL;

    if (!(g_osvex.dwMajorVersion == 5 && g_osvex.dwMinorVersion == 0))
    {
	    HMODULE hmIPHlpApi = NULL;
	    lpfnAllocateAndGetTcpExTableFromStack AllocateAndGetTcpExTableFromStack = NULL;
	    lpfnAllocateAndGetUdpExTableFromStack AllocateAndGetUdpExTableFromStack = NULL;

		INC_SYS_CALL_COUNT(LoadLibraryEx); // Needed to be INC TKH
	    hmIPHlpApi = LoadLibraryEx(_T("iphlpapi.dll"), NULL, 0);

	    if(hmIPHlpApi == NULL)
	    {
		    // Couldn't load the IPHelper API so we're screwed here, we're not about
		    // to go hook the network stack manually... yet.
	        AppLog.Write("Could not load iphlpapi.dll", WSLMSG_STATUS);
	        AppLog.Write("IP Helper not available.", WSLMSG_STATUS);
	    }
        else
        {
	        // These should be available from at least Win2k through WinXP
            INC_SYS_CALL_COUNT(GetProcAddress); 
	        AllocateAndGetTcpExTableFromStack = (lpfnAllocateAndGetTcpExTableFromStack)GetProcAddress(hmIPHlpApi, "AllocateAndGetTcpExTableFromStack");
            INC_SYS_CALL_COUNT(GetProcAddress); 
	        AllocateAndGetUdpExTableFromStack = (lpfnAllocateAndGetUdpExTableFromStack)GetProcAddress(hmIPHlpApi, "AllocateAndGetUdpExTableFromStack");

	        // These should be available from WinXP SP2 onwards...
            INC_SYS_CALL_COUNT(GetProcAddress); 
	        MyGetExtendedTcpTable = (lpfnGetExtendedTcpTable)GetProcAddress(hmIPHlpApi, "GetExtendedTcpTable");
            INC_SYS_CALL_COUNT(GetProcAddress); 
	        MyGetExtendedUdpTable = (lpfnGetExtendedUdpTable)GetProcAddress(hmIPHlpApi, "GetExtendedUdpTable");
	        // I'm only commenting the first two blocks here because everything else is redundant (unless otherwise commented)
	        // Prefer the "best" verstion of the functions that are available
	        if(MyGetExtendedTcpTable)
	        {
                dwSizeOfTable = 0;
                INC_SYS_CALL_COUNT(GetExtendedTcpTable); 
		        dwRetVal = MyGetExtendedTcpTable(pTCPTableOwnerPID, &dwSizeOfTable, FALSE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);
		        // The only error that we should get at this point is an insufficient buffer.
		        if(dwRetVal == ERROR_INSUFFICIENT_BUFFER)
		        {
                    INC_SYS_CALL_COUNT(LocalAlloc); 
			        pTCPTableOwnerPID = (MIB_TCPTABLE_OWNER_PID*)LocalAlloc(LPTR, dwSizeOfTable);
                    INC_SYS_CALL_COUNT(GetExtendedTcpTable); 
			        dwRetVal = MyGetExtendedTcpTable(pTCPTableOwnerPID, &dwSizeOfTable, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0);

			        // Did we get everything okay?
			        if(dwRetVal == ERROR_SUCCESS)
			        {
				        // Loop through and enumerate the table into vecOpenPorts
				        for(DWORD i = 0; i < pTCPTableOwnerPID->dwNumEntries; i++)
				        {
					        memset(&pi, 0, sizeof(PortInfo));
					        pi.dwType = PROTO_TCP;
					        pin_addr = (in_addr*)&(pTCPTableOwnerPID->table[i].dwLocalAddr);
							
							


#ifdef _UNICODE
							len =mbstowcs(NULL,inet_ntoa(*pin_addr),0);
							waddr=new wchar_t[len+1];
							memset(waddr,0,len+1);
							mbstowcs(waddr,inet_ntoa(*pin_addr),len+1);
							
							_stprintf_s(pi.szLocalIPv4, sizeof(pi.szLocalIPv4)/sizeof(TCHAR), _T("%s"), waddr);
							delete [] waddr;							
#else


					        _stprintf_s(pi.szLocalIPv4, sizeof(pi.szLocalIPv4), _T("%s"), inet_ntoa(*pin_addr));
#endif

					        pi.dwLocalPort = (DWORD)ntohs((u_short)pTCPTableOwnerPID->table[i].dwLocalPort);
					        pin_addr = (in_addr*)&(pTCPTableOwnerPID->table[i].dwRemoteAddr);
							

#ifdef _UNICODE
							len =mbstowcs(NULL,inet_ntoa(*pin_addr),0);
							waddr=new wchar_t[len+1];
							memset(waddr,0,len+1);
							mbstowcs(waddr,inet_ntoa(*pin_addr),len+1);
							_stprintf_s(pi.szRemoteIPv4, sizeof(pi.szRemoteIPv4)/sizeof(TCHAR), _T("%s"), waddr);
							delete [] waddr;							
#else

							_stprintf_s(pi.szRemoteIPv4, sizeof(pi.szRemoteIPv4), _T("%s"), inet_ntoa(*pin_addr));
					        
#endif
							
					        
					        pi.dwRemotePort = (DWORD)ntohs((u_short)pTCPTableOwnerPID->table[i].dwRemotePort);
					        pi.dwState = pTCPTableOwnerPID->table[i].dwState;
					        pi.dwOwnerPID = pTCPTableOwnerPID->table[i].dwOwningPid;
					        vecOpenPorts.push_back(pi);
				        }
			        }
			        else
			        {
	                    AppLog.Write("Could obtain TCP port information.", WSLMSG_STATUS);
				        // If it didn't work, knock it out so that we can try the fallback.
				        MyGetExtendedUdpTable = NULL;
			        }

			        // Free the TCP Table since it was allocated anyway.
                    INC_SYS_CALL_COUNT(LocalFree); 
			        LocalFree(pTCPTableOwnerPID);
		        }
		        else
		        {
			        // If it didn't work, knock it out so that we can try the fallback.  Deja Vu.
			        MyGetExtendedUdpTable = NULL;
		        }
	        }

	        if(MyGetExtendedUdpTable)
	        {
                dwSizeOfTable = 0;
                INC_SYS_CALL_COUNT(GetExtendedUdpTable); 
		        dwRetVal = MyGetExtendedUdpTable(pUDPTableOwnerPID, &dwSizeOfTable, FALSE, AF_INET, UDP_TABLE_OWNER_PID, 0);
        		
		        if(dwRetVal == ERROR_INSUFFICIENT_BUFFER)
		        {
                    INC_SYS_CALL_COUNT(LocalAlloc); 
			        pUDPTableOwnerPID = (MIB_UDPTABLE_OWNER_PID*)LocalAlloc(LPTR, dwSizeOfTable);
                    INC_SYS_CALL_COUNT(GetExtendedUdpTable); 
			        dwRetVal = MyGetExtendedUdpTable(pUDPTableOwnerPID, &dwSizeOfTable, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0);

			        if(dwRetVal == ERROR_SUCCESS)
			        {
				        // There's a lot less data in the UDP table...
				        for(DWORD i = 0; i < pUDPTableOwnerPID->dwNumEntries; i++)
				        {
					        memset(&pi, 0, sizeof(PortInfo));
					        pi.dwType = PROTO_UDP;
					        pin_addr = (in_addr*)&(pUDPTableOwnerPID->table[i].dwLocalAddr);
							
#ifdef _UNICODE
							len =mbstowcs(NULL,inet_ntoa(*pin_addr),0);
							waddr=new wchar_t[len+1];
							memset(waddr,0,len+1);
							mbstowcs(waddr,inet_ntoa(*pin_addr),len+1);
							_stprintf_s(pi.szLocalIPv4, sizeof(pi.szLocalIPv4)/sizeof(TCHAR), _T("%s"), waddr);
							delete [] waddr;							
#else
							_stprintf_s(pi.szLocalIPv4, sizeof(pi.szLocalIPv4), _T("%s"), inet_ntoa(*pin_addr));

#endif
					        
					        pi.dwLocalPort = (DWORD)ntohs((u_short)pUDPTableOwnerPID->table[i].dwLocalPort);
					        pi.dwOwnerPID = pUDPTableOwnerPID->table[i].dwOwningPid;
					        vecOpenPorts.push_back(pi);
				        }
			        }
			        else
			        {
	                    AppLog.Write("Could obtain UDP port information.", WSLMSG_STATUS);
				        MyGetExtendedUdpTable = NULL;
			        }

                    INC_SYS_CALL_COUNT(LocalFree); 
			        LocalFree(pUDPTableOwnerPID);
		        }
		        else
		        {
			        MyGetExtendedUdpTable = NULL;
		        }
	        }
        }
    }

    // default case that also handles win2k
    if ((!MyGetExtendedTcpTable) || (g_osvex.dwMajorVersion == 5 && g_osvex.dwMinorVersion == 0))
    {
        MIB_TCPTABLE *pTCPTable;
        dwSizeOfTable = 0;

        // w2k uses a different functions for collection this data
        INC_SYS_CALL_COUNT(GetTcpTable); 
	    if (GetTcpTable(NULL, &dwSizeOfTable, FALSE) == ERROR_INSUFFICIENT_BUFFER)
        {
            INC_SYS_CALL_COUNT(LocalAlloc); 
		    pTCPTable = (MIB_TCPTABLE*)LocalAlloc(LPTR, dwSizeOfTable);
            if (pTCPTable)
            {
                INC_SYS_CALL_COUNT(GetTcpTable); 
	            dwRetVal = GetTcpTable(pTCPTable, &dwSizeOfTable, TRUE);
                if (dwRetVal == ERROR_SUCCESS)
                {
				    // Loop through and enumerate the table into vecOpenPorts
				    for(DWORD i = 0; i < pTCPTable->dwNumEntries; i++)
				    {
					    memset(&pi, 0, sizeof(PortInfo));
					    pi.dwType = PROTO_TCP;
					    pin_addr = (in_addr*)&(pTCPTable->table[i].dwLocalAddr);

						
#ifdef _UNICODE
							len =mbstowcs(NULL,inet_ntoa(*pin_addr),0);
							waddr=new wchar_t[len+1];
							memset(waddr,0,len+1);
							mbstowcs(waddr,inet_ntoa(*pin_addr),len+1);
							_stprintf_s(pi.szLocalIPv4, sizeof(pi.szLocalIPv4)/sizeof(TCHAR), _T("%s"), waddr);
							delete [] waddr;							
#else
					    _stprintf_s(pi.szLocalIPv4, sizeof(pi.szLocalIPv4), _T("%s"), inet_ntoa(*pin_addr));

#endif

					    pi.dwLocalPort = (DWORD)ntohs((u_short)pTCPTable->table[i].dwLocalPort);
					    pin_addr = (in_addr*)&(pTCPTable->table[i].dwRemoteAddr);

						
#ifdef _UNICODE
							len =mbstowcs(NULL,inet_ntoa(*pin_addr),0);
							waddr=new wchar_t[len+1];
							memset(waddr,0,len+1);
							mbstowcs(waddr,inet_ntoa(*pin_addr),len+1);
							_stprintf_s(pi.szRemoteIPv4, sizeof(pi.szRemoteIPv4)/sizeof(TCHAR), _T("%s"),waddr);
							delete [] waddr;							
#else
						    _stprintf_s(pi.szRemoteIPv4, sizeof(pi.szRemoteIPv4), _T("%s"), inet_ntoa(*pin_addr));
							
						

#endif
						
					    
					    pi.dwRemotePort = (DWORD)ntohs((u_short)pTCPTable->table[i].dwRemotePort);
					    pi.dwState = pTCPTable->table[i].dwState;
					    pi.dwOwnerPID = 0;
					    vecOpenPorts.push_back(pi);
				    }
                }
                else
                {
                    AppLog.Write("Could obtain TCP port information.", WSLMSG_STATUS);
                }
                INC_SYS_CALL_COUNT(LocalFree); 
                LocalFree(pTCPTable);
            }
        }
    }

    // default case that also handles win2k
    if ((!MyGetExtendedUdpTable) || (g_osvex.dwMajorVersion == 5 && g_osvex.dwMinorVersion == 0))
    {
        MIB_UDPTABLE *pUDPTable;
        dwSizeOfTable = 0;
        // w2k uses a different functions for collection this data
        INC_SYS_CALL_COUNT(GetUdpTable); 
	    if (GetUdpTable(NULL, &dwSizeOfTable, FALSE) == ERROR_INSUFFICIENT_BUFFER)
        {
            INC_SYS_CALL_COUNT(LocalAlloc); 
		    pUDPTable = (MIB_UDPTABLE*)LocalAlloc(LPTR, dwSizeOfTable);
            if (pUDPTable)
            {
                INC_SYS_CALL_COUNT(GetUdpTable); 
	            dwRetVal = GetUdpTable(pUDPTable, &dwSizeOfTable, TRUE);
                if (dwRetVal == ERROR_SUCCESS)
                {
				    // Loop through and enumerate the table into vecOpenPorts
				    for(DWORD i = 0; i < pUDPTable->dwNumEntries; i++)
				    {
					    memset(&pi, 0, sizeof(PortInfo));
					    pi.dwType = PROTO_UDP;
					    pin_addr = (in_addr*)&(pUDPTable->table[i].dwLocalAddr);
						
#ifdef _UNICODE
							len =mbstowcs(NULL,inet_ntoa(*pin_addr),0);
							waddr=new wchar_t[len+1];
							memset(waddr,0,len+1);
							mbstowcs(waddr,inet_ntoa(*pin_addr),len+1);
					       _stprintf_s(pi.szLocalIPv4, sizeof(pi.szLocalIPv4)/sizeof(TCHAR), _T("%s"), waddr);
							delete [] waddr;							
#else
						_stprintf_s(pi.szLocalIPv4, sizeof(pi.szLocalIPv4)/sizeof(TCHAR), _T("%s"), inet_ntoa(*pin_addr));
						

#endif
						
					    

					    pi.dwLocalPort = (DWORD)ntohs((u_short)pUDPTable->table[i].dwLocalPort);
					    vecOpenPorts.push_back(pi);
				    }
                }
                else
                {
                    AppLog.Write("Could obtain UDP port information.", WSLMSG_STATUS);
                }

                INC_SYS_CALL_COUNT(LocalFree); 
                LocalFree(pUDPTable);
            }
        }
    }
#if 0
    }
	    // Either GetExtendedTcpTable doesn't exist or is broken, so try a fallback.
	    // This "should" at the very least work from XP through Win2K...
	    if(AllocateAndGetTcpExTableFromStack && !GetExtendedTcpTable)
	    {
            INC_SYS_CALL_COUNT(GetProcessHeap); 
            INC_SYS_CALL_COUNT(AllocateAndGetTcpExTableFromStack); 
		    dwRetVal = AllocateAndGetTcpExTableFromStack(&tcpextable, TRUE, GetProcessHeap(), 0, AF_INET);

		    // Make sure it successfully allocated the TCP Table.
		    if(tcpextable)
		    {
			    // Make sure it actually worked...
			    if(dwRetVal == ERROR_SUCCESS)
			    {
				    pTCPTableOwnerPID = (MIB_TCPTABLE_OWNER_PID*)tcpextable;

				    for(DWORD i = 0; i < pTCPTableOwnerPID->dwNumEntries; i++)
				    {
					    memset(&pi, 0, sizeof(PortInfo));
					    pi.dwType = PROTO_TCP;
					    pin_addr = (in_addr*)&(pTCPTableOwnerPID->table[i].dwLocalAddr);
					    _stprintf_s(pi.szLocalIPv4, sizeof(pi.szLocalIPv4), _T("%s"), inet_ntoa(*pin_addr));
					    pi.dwLocalPort = (DWORD)ntohs((short)pTCPTableOwnerPID->table[i].dwLocalPort);
					    pin_addr = (in_addr*)&(pTCPTableOwnerPID->table[i].dwRemoteAddr);
					    _stprintf_s(pi.szRemoteIPv4, sizeof(pi.szRemoteIPv4), _T("%s"), inet_ntoa(*pin_addr));
					    pi.dwRemotePort = (DWORD)ntohs((u_short)pTCPTableOwnerPID->table[i].dwRemotePort);
					    pi.dwState = pTCPTableOwnerPID->table[i].dwState;
					    pi.dwOwnerPID = pTCPTableOwnerPID->table[i].dwOwningPid;
					    vecOpenPorts.push_back(pi);			
				    }
			    }

                INC_SYS_CALL_COUNT(GetProcessHeap); 
                INC_SYS_CALL_COUNT(HeapFree); 
			    HeapFree(GetProcessHeap(), 0, tcpextable);
		    }
	    }

	    dwSizeOfTable = 0;

	    if(AllocateAndGetUdpExTableFromStack && !GetExtendedUdpTable)
	    {
			INC_SYS_CALL_COUNT(GetProcessHeap); // Needed to be INC TKH
            INC_SYS_CALL_COUNT(AllocateAndGetUdpExTableFromStack); 
		    dwRetVal = AllocateAndGetUdpExTableFromStack(&udpextable, TRUE, GetProcessHeap(), 0, AF_INET);

		    if(udpextable)
		    {
			    if(dwRetVal == ERROR_SUCCESS)
			    {
				    pUDPTableOwnerPID = (MIB_UDPTABLE_OWNER_PID*)udpextable;

				    for(DWORD i = 0; i < pUDPTableOwnerPID->dwNumEntries; i++)
				    {
					    memset(&pi, 0, sizeof(PortInfo));
					    pi.dwType = PROTO_UDP;
					    pin_addr = (in_addr*)&(pUDPTableOwnerPID->table[i].dwLocalAddr);
					    _stprintf_s(pi.szLocalIPv4, sizeof(pi.szLocalIPv4), _T("%s"), inet_ntoa(*pin_addr));
					    pi.dwLocalPort = (DWORD)ntohs((u_short)pUDPTableOwnerPID->table[i].dwLocalPort);
					    pi.dwOwnerPID = pUDPTableOwnerPID->table[i].dwOwningPid;
					    vecOpenPorts.push_back(pi);
				    }
			    }

                INC_SYS_CALL_COUNT(GetProcessHeap); 
                INC_SYS_CALL_COUNT(HeapFree); 
			    HeapFree(GetProcessHeap(), 0, udpextable);
		    }
	    }
    }
#endif
	return 1;
}

int NetworkInfo::GetRoutingTable(void)
{
	return 0;
}

int NetworkInfo::GetARPTable(void)
{
	return 0;
}

int NetworkInfo::GetMACAddress(void)
{
	return 0;
}

int NetworkInfo::EnumerateAdapters(void)
{
	MIB_IFTABLE *pIfTable = NULL;
	LPTSTR pszTempStr = NULL;
	LPTSTR pszTempStr2 = NULL;
	DWORD dwIfTableSize = 0;
	DWORD dwRetVal = 0;
	DWORD x = 0;
	NetworkInterface ni;
	vector<wstring> iflist;
	int duplicate = 0;

    INC_SYS_CALL_COUNT(GetIfTable); 
	dwRetVal = GetIfTable(NULL, &dwIfTableSize, FALSE);
	if((dwRetVal == ERROR_INSUFFICIENT_BUFFER) && dwIfTableSize)
	{
        INC_SYS_CALL_COUNT(GetProcessHeap); 
        INC_SYS_CALL_COUNT(HeapAlloc); 
		pIfTable = (MIB_IFTABLE*) HeapAlloc(GetProcessHeap(), 0, dwIfTableSize);
		if(!pIfTable)
        {
			return 0;
        }
		else
        {
            INC_SYS_CALL_COUNT(GetIfTable); 
			dwRetVal = GetIfTable(pIfTable, &dwIfTableSize, TRUE);
        }
	}


	if(dwRetVal != ERROR_SUCCESS)
	{
		INC_SYS_CALL_COUNT(HeapFree); // Needed to be INC TKH
		INC_SYS_CALL_COUNT(GetProcessHeap); // Needed to be INC TKH
	    HeapFree(GetProcessHeap(), 0, pIfTable);
		return 0;
	}

	for(DWORD i = 0; i < pIfTable->dwNumEntries; i++)
	{
		x=0;//Fix the error in the first digits of MAC address.
		duplicate=0;
		memset(&ni, 0, sizeof(NetworkInterface));

		ni.dwIfIndex = pIfTable->table[i].dwIndex;
		ni.dwIfType = pIfTable->table[i].dwType;
		ni.bEnabled = pIfTable->table[i].dwAdminStatus;
		ni.dwStatus = pIfTable->table[i].dwOperStatus;

		// Get the MAC address
		if(pIfTable->table[i].dwPhysAddrLen > 0)
		{
            int iLength;
			// Build a MAC string.  It's ugly, but hopefully works.
			// 3 chars for each byte + numbytes - 1 dashes + a null terminator.
            iLength = sizeof(TCHAR) * ((((pIfTable->table[i]).dwPhysAddrLen) * 3) + (((pIfTable->table[i]).dwPhysAddrLen) - 1) + 1);
			pszTempStr = (LPTSTR)malloc(iLength*sizeof(TCHAR));

			if(pszTempStr)
			{
				memset(pszTempStr, 0, iLength);

				// First element.
				_stprintf_s((LPTSTR)((TCHAR*)pszTempStr), iLength, _T("%02X"), pIfTable->table[i].bPhysAddr[x]);
				pszTempStr2 = pszTempStr + 2;
                iLength -= 2;

				for(x = 1; x < pIfTable->table[i].dwPhysAddrLen; x++)
				{
					// Pointer-foo!
					_stprintf_s(pszTempStr2, iLength, _T("-%02X"), pIfTable->table[i].bPhysAddr[x]);
					pszTempStr2 += 3;	// 3 TCHARs!!!
                    iLength -= 3;
				}

				_tcscpy_s(ni.szMACAddr, sizeof(ni.szMACAddr)/sizeof(TCHAR), pszTempStr);
				free(pszTempStr);
			}
		}
		else
		{
			// Fix for the "empty" MAC address.
			// Disabled adapters don't return a MAC.
			_tcscpy_s(ni.szMACAddr, 16, _T("Unavailable"));
		}


		for(vector<wstring>::iterator it=iflist.begin();it!=iflist.end();it++)
		{
			wstring currentMac(ni.szMACAddr);
			if(*it == currentMac)
			{
				duplicate = 1;
			}
			
		}
		if(!duplicate)
		{
			iflist.push_back(ni.szMACAddr);
			vecInterfaces.push_back(ni);
		}
	}

    INC_SYS_CALL_COUNT(GetProcessHeap); 
    INC_SYS_CALL_COUNT(HeapFree); 
	HeapFree(GetProcessHeap(), 0, pIfTable);

	return 1;
}

int NetworkInfo::CapturePackets(LPCTSTR szFileName, int Seconds)
{
	return 0;
}

int NetworkInfo::WriteXML(HashedXMLFile &repository)
{
	TCHAR szTempStr[MAX_PATH];
    TCHAR *szValue;

	
	// Create the interface parent
    repository.OpenNode("Interfaces");
//	xmlIfParent = xmlCurrentNode = xmlTopNode.addChild(_T("Interfaces"));
	
	

	for(DWORD i = 0; i < vecInterfaces.size(); i++)
	{
		// Start an interface entry item
        repository.OpenNode("IfEntry");
//		xmlTempParent = xmlIfParent.addChild(_T("IfEntry"));
		
		// Interface number
		_itot_s(vecInterfaces[i].dwIfIndex, szTempStr, MAX_PATH, 10);
        repository.WriteNodeWithValue("InterfaceID", szTempStr);

		// Interface type
		switch(vecInterfaces[i].dwIfType)
		{
		case 1:
			szValue = _T("Other Unknown Adapter");
			break;
		case 6:
			szValue = _T("Ethernet Adapter");
			break;
		case 23:
			szValue = _T("PPP Adapter");
			break;
		case 24:
			szValue = _T("Software Loopback Adapter");
			break;
		case 71:
			szValue = _T("802.11 Wireless Adapter");
			break;
		case 131:
			szValue = _T("Tunneling Adapter");
			break;
		case 144:
			szValue = _T("IEEE1394 (Firewire) Adapter");
			break;
		default:
			szValue = _T("Unknown Adapter");
			break;
		}
        repository.WriteNodeWithValue("InterfaceType", szValue);

		// MAC Address
        repository.WriteNodeWithValue("MAC", vecInterfaces[i].szMACAddr);
	
		// Enabled state
		if(vecInterfaces[i].bEnabled)
           repository.WriteNodeWithValue("Status", "Enabled");
		else
           repository.WriteNodeWithValue("Status", "Disabled");


		switch(vecInterfaces[i].dwStatus)
		{
            case IF_OPER_STATUS_NON_OPERATIONAL:
                szValue = _T("Non Operational");
                break;
            case IF_OPER_STATUS_UNREACHABLE:
                szValue = _T("Unreasonable");
                break;
            case IF_OPER_STATUS_DISCONNECTED:
                szValue = _T("Disconnected");
                break;
            case IF_OPER_STATUS_CONNECTING:
                szValue = _T("Connecting");
                break;
            case IF_OPER_STATUS_CONNECTED:
                szValue = _T("Connected");
                break;
            case IF_OPER_STATUS_OPERATIONAL:
                szValue = _T("Operational");
                break;
            default:
                szValue = _T("Unknown status");
                break;
		}
        repository.WriteNodeWithValue("State", szValue);

        repository.CloseNode("IfEntry");

	}
    repository.CloseNode("Interfaces");

	// Create the portmap parent
    if (vecOpenPorts.size())
    {
       repository.OpenNode("PortMap");
//	xmlPortsParent = xmlCurrentNode = xmlTopNode.addChild(_T("PortMap"));

	    for(DWORD i = 0; i < vecOpenPorts.size(); i++)
	    {
		    // Reset the parent and create a new port entry
            repository.OpenNode("PortEntry");

            // This should all be fairly self explanitory
		    if(vecOpenPorts[i].dwType == PROTO_TCP)
                repository.WriteNodeWithValue("Type", "TCP");
		    else
                repository.WriteNodeWithValue("Type", "UDP");

            repository.WriteNodeWithValue("LocalAddress", vecOpenPorts[i].szLocalIPv4);
   		
		    _itot_s(vecOpenPorts[i].dwLocalPort, szTempStr, sizeof(szTempStr)/sizeof(TCHAR), 10);
            repository.WriteNodeWithValue("LocalPort", szTempStr);
    		
		    if(vecOpenPorts[i].dwType == PROTO_TCP)
		    {

                if (vecOpenPorts[i].dwState == MIB_TCP_STATE_LISTEN)
                {
                    repository.WriteNodeWithValue("RemoteAddress", "0.0.0.0");
                    repository.WriteNodeWithValue("RemotePort", "0");
                }
                else
                {
                    repository.WriteNodeWithValue("RemoteAddress", vecOpenPorts[i].szRemoteIPv4);
			        _itot_s(vecOpenPorts[i].dwRemotePort, szTempStr, sizeof(szTempStr)/sizeof(TCHAR), 10);
                    repository.WriteNodeWithValue("RemotePort", szTempStr);
                }

			    switch(vecOpenPorts[i].dwState)
			    {
			    case MIB_TCP_STATE_CLOSED:
                    szValue = _T("CLOSED");
				    break;
			    case MIB_TCP_STATE_LISTEN:
				    szValue = _T("LISTENING");
				    break;
			    case MIB_TCP_STATE_SYN_SENT:
				    szValue = _T("SYN SENT");
				    break;
			    case MIB_TCP_STATE_SYN_RCVD:
				    szValue = _T("SYN RECEIVED");
				    break;
			    case MIB_TCP_STATE_ESTAB:
				    szValue = _T("ESTABLISHED");
				    break;
			    case MIB_TCP_STATE_FIN_WAIT1:
				    szValue = _T("FIN-WAIT-1");
				    break;
			    case MIB_TCP_STATE_FIN_WAIT2:
				    szValue = _T("FIN-WAIT-2");
				    break;
			    case MIB_TCP_STATE_CLOSE_WAIT:
				    szValue = _T("CLOSE-WAIT");
				    break;
			    case MIB_TCP_STATE_CLOSING:
				    szValue = _T("CLOSING");
				    break;
			    case MIB_TCP_STATE_LAST_ACK:
				    szValue = _T("LAST-ACK");
				    break;
			    case MIB_TCP_STATE_TIME_WAIT:
				    szValue = _T("TIME-WAIT");
				    break;
			    case MIB_TCP_STATE_DELETE_TCB:
				    szValue = _T("DELETE TCB");
				    break;
			    default:
				    szValue = _T("UNKNOWN");
			    }
                repository.WriteNodeWithValue("State", szValue);
		    }

		    _itot_s(vecOpenPorts[i].dwOwnerPID, szTempStr, sizeof(szTempStr)/sizeof(TCHAR), 10);
            repository.WriteNodeWithValue("PID", szTempStr);
            repository.CloseNode("PortEntry");
	    }
        repository.CloseNode("PortMap");
    }


	return 0;
}

#undef _UNICODE
#undef UNICODE
