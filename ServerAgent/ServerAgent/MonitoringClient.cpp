#include "stdafx.h"
#include "CommonProtocol.h"
#include "CpuUsage.h"
#include "LanClient.h"
#include "MonitoringClient.h"
#include "Agent.h"
#include "main.h"



void CMonitoringClient::OnEnterJoinServer(void)
{
	SendPacket_MonitorServerLogin();

	wprintf(L"Connect to Monitoring Server success.\n");
	return;
}

void CMonitoringClient::OnLeaveServer(void)
{
	wprintf(L"Disconnect From Monitoring Server\n");
	return;
}

void CMonitoringClient::OnRecv(CPacket *pPacket)
{
	WORD type;

	*pPacket >> type;

	switch (type)
	{
		case en_PACKET_CS_MONITOR_TOOL_SERVER_CONTROL:
			PacketProc_ServerControl(pPacket);
			break;

		default :
			break;
	}

	return;
}

void CMonitoringClient::OnSend(int iSendSize)
{
	return;
}

void CMonitoringClient::SendPacket_MonitorServerLogin(void)
{
	CPacket *pPacket = CPacket::Alloc();
	pPacket->SetHeaderSize(2);

	MakePacket_MonitorServerLogin(pPacket);
	SendPacket(pPacket);
	pPacket->Free();
}

void CMonitoringClient::MakePacket_MonitorServerLogin(CPacket *pInPacket)
{
	*pInPacket << (WORD)en_PACKET_SS_MONITOR_LOGIN;
	*pInPacket << (BYTE)dfMONITOR_SERVER_TYPE_AGENT;
	pInPacket->Enqueue((char *)this->_szServerGroupName, 64);

	HEADER header;
	header.wSize = (WORD)(pInPacket->GetPayloadUseSize());
	pInPacket->InputHeader((char *)&header, sizeof(header));

	return;
}

void CMonitoringClient::PacketProc_ServerControl(CPacket *pRecvPacket)
{
	BYTE serverType;
	BYTE control;

	*pRecvPacket >> serverType >> control;

	if (dfMONITOR_SERVER_TYPE_GAME == serverType)
	{
		switch (control)
		{
			case dfMONITOR_SERVER_CONTROL_RUN:
				g_Agent.GameServer_Start();
				break;

			case dfMONITOR_SERVER_CONTROL_SHUTDOWN:
				g_Agent.GameServer_Shutdown();
				break;

			case dfMONITOR_SERVER_CONTROL_TERMINATE:
				g_Agent.GameServer_Terminate();
				break;

			default:
				break;
		}
	}
	else if (dfMONITOR_SERVER_TYPE_CHAT == serverType)
	{
		switch (control)
		{
		case dfMONITOR_SERVER_CONTROL_RUN:
			g_Agent.ChatServer_Start();
			break;

		case dfMONITOR_SERVER_CONTROL_TERMINATE:
			g_Agent.ChatServer_Terminate();
			break;

		default:
			break;
		}
	}
}