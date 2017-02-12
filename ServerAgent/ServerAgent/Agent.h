#pragma once

class CAgent
{
public:
	CAgent();

	bool GameServer_Start();
	bool GameServer_Shutdown();
	bool GameServer_Terminate();

	bool ChatServer_Start();
	bool ChatServer_Terminate();

	bool InitialPdh(void);
	bool UpdatePdh(void);

private:
	void SendPacket_AvailableMemory();
	void SendPacket_NonpagedMemory();

	void SendPacket_ReceivedBytes();
	void SendPacket_SentBytes();

private:
	CMonitoringClient _lanClient_Monitoring;

	CCpuUsage _usageGameServer;
	CCpuUsage _usageChatServer;

	HANDLE _hGameServer;
	HANDLE _hChatServer;

	PDH_HQUERY _pdhQuery;

	PDH_HCOUNTER _pdhCounterThreadCounterGameServer;
	PDH_HCOUNTER _pdhCounterWorkingMemoryGameServer;

	long	_pdhValueThreadCounterGameServer;
	double	_pdhValueWorkingMemoryGameServer;

	PDH_HCOUNTER _pdhCounterThreadCounterChatServer;
	PDH_HCOUNTER _pdhCounterWorkingMemoryChatServer;

	long	_pdhValueThreadCounterChatServer;
	double	_pdhValueWorkingMemoryChatServer;

	PDH_HCOUNTER _pdhCounterNonpagedMemory;
	PDH_HCOUNTER _pdhCounterAvailableMemory;

	long _pdhValueNonpagedMemory;
	long _pdhValueAvailableMemory;

	PDH_HCOUNTER _pdhCounterNetworkRecv;
	PDH_HCOUNTER _pdhCounterNetworkSend;

	long _pdhValueReceivedBytes;
	long _pdhValueSentBytes;
};