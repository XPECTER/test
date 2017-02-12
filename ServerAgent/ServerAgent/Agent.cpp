#include "stdafx.h"
#include "CpuUsage.h"
#include "LanClient.h"
#include "MonitoringClient.h"
#include "CommonProtocol.h"
#include "Agent.h"
#include "main.h"


CAgent::CAgent()
{
	this->_hGameServer = INVALID_HANDLE_VALUE;
	this->_hChatServer = INVALID_HANDLE_VALUE;

	this->_pdhValueNonpagedMemory = 0;
	this->_pdhValueAvailableMemory = 0;
	this->_pdhValueReceivedBytes = 0;
	this->_pdhValueSentBytes = 0;
}

bool CAgent::InitialPdh(void)
{
	if (ERROR_SUCCESS != PdhOpenQuery(NULL, NULL, &this->_pdhQuery))
	{
		return false;
	}

	wchar_t szQuery[256] = { 0, };
	wchar_t fileName[128] = { 0, };
	wchar_t *token = nullptr, *nextToken = nullptr;

	wcscpy_s(fileName, 128, g_ConfigData.szGameServerFileName);
	token = wcstok_s(fileName, L".", &nextToken);
	
	StringCbPrintf(szQuery, 128, L"\\Process(%s)\\Thread Count", token);
	PdhAddCounter(this->_pdhQuery, szQuery, NULL, &this->_pdhCounterThreadCounterGameServer);
	
	StringCbPrintf(szQuery, 128, L"\\Process(%s)\\Working Set", token);
	PdhAddCounter(this->_pdhQuery, szQuery, NULL, &this->_pdhCounterWorkingMemoryGameServer);

	wcscpy_s(fileName, 128, g_ConfigData.szChatServerFileName);
	token = wcstok_s(fileName, L".", &nextToken);

	StringCbPrintf(szQuery, 128, L"\\Process(%s)\\Thread Count", token);
	PdhAddCounter(this->_pdhQuery, szQuery, NULL, &this->_pdhCounterThreadCounterChatServer);
	
	StringCbPrintf(szQuery, 128, L"\\Process(%s)\\Working Set", token);
	PdhAddCounter(this->_pdhQuery, szQuery, NULL, &this->_pdhCounterWorkingMemoryChatServer);

	PdhAddCounter(this->_pdhQuery, L"\\Memory\\Available MBytes", NULL, &this->_pdhCounterAvailableMemory);
	PdhAddCounter(this->_pdhQuery, L"\\Memory\\Pool Nonpaged Bytes", NULL, &this->_pdhCounterNonpagedMemory);

	PdhAddCounter(this->_pdhQuery, L"\\Network Interface(*)\\Bytes Received/sec", NULL, &this->_pdhCounterNetworkRecv);
	PdhAddCounter(this->_pdhQuery, L"\\Network Interface(*)\\Bytes Sent/sec", NULL, &this->_pdhCounterNetworkSend);

	return true;
}

bool CAgent::UpdatePdh(void)
{
	this->_lanClient_Monitoring.Connect();
	
	PDH_STATUS status;
	PDH_FMT_COUNTERVALUE CounterValue;
	PdhCollectQueryData(_pdhQuery);

	status = PdhGetFormattedCounterValue(this->_pdhCounterAvailableMemory, PDH_FMT_LONG, NULL, &CounterValue);
	if (0 == status)
	{
		this->_pdhValueAvailableMemory = CounterValue.longValue;
		this->SendPacket_AvailableMemory();
	}

	status = PdhGetFormattedCounterValue(this->_pdhCounterNonpagedMemory, PDH_FMT_LONG, NULL, &CounterValue);
	if (0 == status)
	{
		this->_pdhValueNonpagedMemory = CounterValue.longValue;
		this->SendPacket_NonpagedMemory();
	}

	status = PdhGetFormattedCounterValue(this->_pdhCounterNetworkRecv, PDH_FMT_LONG, NULL, &CounterValue);
	if (0 == status)
	{
		this->_pdhValueReceivedBytes = CounterValue.longValue;
		this->SendPacket_ReceivedBytes();
	}
		

	status = PdhGetFormattedCounterValue(this->_pdhCounterNetworkSend, PDH_FMT_LONG, NULL, &CounterValue);
	if (0 == status)
	{
		this->_pdhValueSentBytes = CounterValue.longValue;
		this->SendPacket_SentBytes();
	}

	wprintf(L"Available Memory : %dMBytes\n", this->_pdhValueAvailableMemory);
	wprintf(L"Non Paged Memory : %dMBytes\n\n", this->_pdhValueNonpagedMemory / 1024 / 1024);

	wprintf(L"Received Bytes\t: %dKBytes\n", this->_pdhValueReceivedBytes / 1024);
	wprintf(L"Sent Bytes\t: %dKBytes\n\n", this->_pdhValueSentBytes / 1024);

	if (NULL == FindWindow(NULL, g_ConfigData.szGameServerFileName))
		this->_hGameServer = INVALID_HANDLE_VALUE;

	if (INVALID_HANDLE_VALUE != this->_hGameServer)
	{
		status = PdhGetFormattedCounterValue(this->_pdhCounterThreadCounterGameServer, PDH_FMT_LONG, NULL, &CounterValue);
		if (0 == status)
			this->_pdhValueThreadCounterGameServer = CounterValue.longValue;

		status = PdhGetFormattedCounterValue(this->_pdhCounterWorkingMemoryGameServer, PDH_FMT_DOUBLE, NULL, &CounterValue);
		if (0 == status)
			this->_pdhValueWorkingMemoryGameServer = CounterValue.doubleValue;

		this->_usageGameServer.UpdateCpuTime();

		wprintf(L"GameServer Process Total  : %5.2f%%\n", this->_usageGameServer.ProcessTotal());
		wprintf(L"GameServer Thread Count   : %d\n", this->_pdhValueThreadCounterGameServer);
		wprintf(L"GameServer Working Memory : %5.2fMBytes\n\n", this->_pdhValueWorkingMemoryGameServer / 1024 / 1024);
		//wprintf(L"GameServer Process Kernel : %5.2f\n", this->_usageGameServer.ProcessKernel());
		//wprintf(L"GameServer Process User : %5.2f\n\n", this->_usageGameServer.ProcessUser());
	}
	else
	{
		wprintf(L"");
	}

	if (NULL == FindWindow(NULL, g_ConfigData.szChatServerFileName))
		this->_hChatServer = INVALID_HANDLE_VALUE;

	if (INVALID_HANDLE_VALUE != this->_hChatServer)
	{
		status = PdhGetFormattedCounterValue(this->_pdhCounterThreadCounterChatServer, PDH_FMT_LONG, NULL, &CounterValue);
		if (0 == status)
			this->_pdhValueThreadCounterChatServer = CounterValue.longValue;

		status = PdhGetFormattedCounterValue(this->_pdhCounterWorkingMemoryChatServer, PDH_FMT_DOUBLE, NULL, &CounterValue);
		if (0 == status)
			this->_pdhValueWorkingMemoryChatServer = CounterValue.doubleValue;

		this->_usageChatServer.UpdateCpuTime();

		wprintf(L"ChatServer Process Total  : %5.2f%%\n", this->_usageChatServer.ProcessTotal());
		wprintf(L"ChatServer Thread Count   : %dEA\n", this->_pdhValueThreadCounterChatServer);
		wprintf(L"ChatServer Working Memory : %5.2fMBytes\n\n", this->_pdhValueWorkingMemoryChatServer / 1024 / 1024);
		//wprintf(L"ChatServer Process Kernel : %5.2f\n", this->_usageChatServer.ProcessKernel());
		//wprintf(L"ChatServer Process User : %5.2f\n\n", this->_usageChatServer.ProcessUser());
	}
	else
	{
		wprintf(L"");
	}
	
	return true;
}

bool CAgent::GameServer_Start()
{
	if (INVALID_HANDLE_VALUE != this->_hGameServer)
		return false;

	STARTUPINFO StartInfo;

	ZeroMemory(&StartInfo, sizeof(StartInfo));

	StartInfo.cb = sizeof(StartInfo);
	StartInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USEFILLATTRIBUTE;
	StartInfo.dwFillAttribute = BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
	StartInfo.wShowWindow = SW_MINIMIZE;

	PROCESS_INFORMATION ProcessInfo;

	if (!CreateProcess(g_ConfigData.szGameServerFileName, NULL, NULL, NULL, NULL, CREATE_NEW_CONSOLE, NULL, NULL, &StartInfo, &ProcessInfo))
	{
		return false;
	}
	else
	{
		CloseHandle(ProcessInfo.hThread);

		this->_hGameServer = ProcessInfo.hProcess;
		this->_usageGameServer.ProcessHandleChange(ProcessInfo.hProcess);
		return true;
	}
}

bool CAgent::GameServer_Shutdown()
{
	return true;
}

bool CAgent::GameServer_Terminate()
{
	if (INVALID_HANDLE_VALUE == _hGameServer)
		return false;

	TerminateProcess(this->_hGameServer, 0);
	CloseHandle(this->_hGameServer);

	this->_hGameServer = INVALID_HANDLE_VALUE;
	this->_usageGameServer.ProcessHandleChange(INVALID_HANDLE_VALUE);
	return true;
}

bool CAgent::ChatServer_Start()
{
	if (INVALID_HANDLE_VALUE != this->_hChatServer)
		return false;

	STARTUPINFO StartInfo;

	ZeroMemory(&StartInfo, sizeof(StartInfo));

	StartInfo.cb = sizeof(StartInfo);
	StartInfo.dwFlags = STARTF_USESHOWWINDOW | STARTF_USEFILLATTRIBUTE;
	StartInfo.dwFillAttribute = BACKGROUND_RED | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_INTENSITY;
	StartInfo.wShowWindow = SW_MINIMIZE;

	PROCESS_INFORMATION ProcessInfo;

	if (!CreateProcess(g_ConfigData.szChatServerFileName, NULL, NULL, NULL, NULL, CREATE_NEW_CONSOLE, NULL, NULL, &StartInfo, &ProcessInfo))
	{
		return false;
	}
	else
	{
		CloseHandle(ProcessInfo.hThread);

		this->_hChatServer = ProcessInfo.hProcess;
		this->_usageChatServer.ProcessHandleChange(ProcessInfo.hProcess);
		return true;
	}
}

bool CAgent::ChatServer_Terminate()
{
	if (INVALID_HANDLE_VALUE == _hChatServer)
		return false;

	TerminateProcess(this->_hChatServer, 0);
	CloseHandle(this->_hChatServer);

	this->_hChatServer = INVALID_HANDLE_VALUE;
	this->_usageChatServer.ProcessHandleChange(INVALID_HANDLE_VALUE);
	return true;
}

void CAgent::SendPacket_AvailableMemory()
{
	CPacket *pPacket = CPacket::Alloc();
	pPacket->SetHeaderSize(2);
	*pPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
	*pPacket << (BYTE)dfMONITOR_DATA_TYPE_AGENT_AVAILABLE_MEMORY;
	*pPacket << (int)this->_pdhValueAvailableMemory;
	*pPacket << (int)(time(NULL));

	HEADER header;
	header.wSize = pPacket->GetPayloadUseSize();
	pPacket->InputHeader((char *)&header, sizeof(header));

	this->_lanClient_Monitoring.SendPacket(pPacket);
	pPacket->Free();
}

void CAgent::SendPacket_NonpagedMemory()
{
	CPacket *pPacket = CPacket::Alloc();
	pPacket->SetHeaderSize(2);
	*pPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
	*pPacket << (BYTE)dfMONITOR_DATA_TYPE_AGENT_NONPAGED_MEMORY;
	*pPacket << (int)this->_pdhValueNonpagedMemory;
	*pPacket << (int)(time(NULL));

	HEADER header;
	header.wSize = pPacket->GetPayloadUseSize();
	pPacket->InputHeader((char *)&header, sizeof(header));

	this->_lanClient_Monitoring.SendPacket(pPacket);
	pPacket->Free();
}

void CAgent::SendPacket_ReceivedBytes()
{
	CPacket *pPacket = CPacket::Alloc();
	pPacket->SetHeaderSize(2);
	*pPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
	*pPacket << (BYTE)dfMONITOR_DATA_TYPE_AGENT_NETWORK_RECV;
	*pPacket << (int)this->_pdhValueReceivedBytes;
	*pPacket << (int)(time(NULL));

	HEADER header;
	header.wSize = pPacket->GetPayloadUseSize();
	pPacket->InputHeader((char *)&header, sizeof(header));

	this->_lanClient_Monitoring.SendPacket(pPacket);
	pPacket->Free();
}

void CAgent::SendPacket_SentBytes()
{
	CPacket *pPacket = CPacket::Alloc();
	pPacket->SetHeaderSize(2);
	*pPacket << (WORD)en_PACKET_SS_MONITOR_DATA_UPDATE;
	*pPacket << (BYTE)dfMONITOR_DATA_TYPE_AGENT_NETWORK_SEND;
	*pPacket << (int)this->_pdhValueSentBytes;
	*pPacket << (int)(time(NULL));

	HEADER header;
	header.wSize = pPacket->GetPayloadUseSize();
	pPacket->InputHeader((char *)&header, sizeof(header));

	this->_lanClient_Monitoring.SendPacket(pPacket);
	pPacket->Free();
}