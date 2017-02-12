// ServerAgent.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "CpuUsage.h"
#include "LanClient.h"
#include "MonitoringClient.h"
#include "Agent.h"
#include "main.h"

bool g_bLocked;
time_t OnTime;
tm t;
st_CONFIG g_ConfigData;

CAgent g_Agent;

long CCrashDump::_DumpCount = 0;
CCrashDump crashDump;

CMemoryPoolTLS<CPacket> CPacket::PacketPool(300, true);
BYTE CPacket::_packetCode = 0;
BYTE CPacket::_packetKey_1 = 0;
BYTE CPacket::_packetKey_2 = 0;

int _tmain(int argc, _TCHAR* argv[])
{
	setlocale(LC_ALL, "");

	Init_main();
	g_Agent.InitialPdh();

	while (true)
	{
		system("cls");
		
		PrintState();
		g_Agent.UpdatePdh();
		KeyProcess();

		Sleep(999);
	}

	return 0;
}

bool Init_main()
{
	g_bLocked = true;
	time(&OnTime);
	localtime_s(&t, &OnTime);

	CConfigParser configParser;

	if (!configParser.OpenConfigFile(L"AgentConfig.ini"))
		return false;

	if (!configParser.MoveConfigBlock(L"NETWORK"))
	{
		wprintf_s(L"ConfigParseError : NETWORK Block\n");
		return false;
	}
	else
	{
		configParser.GetValue(L"SERVER_GROUP_NAME", g_ConfigData.szServerGroupName, dfSERVER_GROUP_NAME_LEN);
		
		configParser.GetValue(L"LAN_BIND_IP", g_ConfigData.szBindIP, 16);
		configParser.GetValue(L"LAN_BIND_PORT", &g_ConfigData.iBindPort);
		
		configParser.GetValue(L"MONITORING_SERVER_IP", g_ConfigData.szMonitoringServerIP, 16);
		configParser.GetValue(L"MONITORING_SERVER_PORT", &g_ConfigData.iMonitoringServerPort);
	}

	if (!configParser.MoveConfigBlock(L"SYSTEM"))
	{
		wprintf_s(L"ConfigParseError : SYSTEM Block\n");
		return false;
	}
	else
	{
		configParser.GetValue(L"GAMESERVER_EXE", g_ConfigData.szGameServerFileName, dfGAME_SERVER_NAME_LEN);
		configParser.GetValue(L"CHATSERVER_EXE", g_ConfigData.szChatServerFileName, dfCHAT_SERVER_NAME_LEN);
	}

	return true;
}

void PrintState()
{
	wprintf_s(L"AGENT ON TIME : [%04d-%02d-%02d %02d:%02d:%02d]\n", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
	wprintf_s(L"===============================================================\n");

	if (true == g_bLocked)
	{
		wprintf(L"[U] key : UNLOCK\n");
	}
	else
	{
		wprintf(L"[L] : LOCK\n");
		wprintf(L"[G] : Start GameServer\n");
		wprintf(L"[C] : Start ChatServer\n");
		wprintf(L"[S] : Shutdown GameServer\n");
		wprintf(L"[Q] : Terminate GameServer\n");
		wprintf(L"[X] : Terminate ChatServer\n");
	}

	wprintf_s(L"===============================================================\n");
	return;
}

void KeyProcess()
{
	char key;

	if (0 != _kbhit())
	{
		key = _getch();

		if (true == g_bLocked)
		{
			if ('U' == key || 'u' == key)
				g_bLocked = false;
		}
		else
		{
			switch (key)
			{
				case 'L':
				case 'l':
				{
					g_bLocked = true;
					break;
				}

				case 'G':
				case 'g':
				{
					g_Agent.GameServer_Start();
					break;
				}

				case 'C':
				case 'c':
				{
					g_Agent.ChatServer_Start();
					break;
				}

				case 'S':
				case 's':
				{
					
					break;
				}

				case 'Q':
				case 'q':
				{
					g_Agent.GameServer_Terminate();
					break;
				}

				case 'X':
				case 'x':
				{
					g_Agent.ChatServer_Terminate();
					break;
				}
			}
		}
	}
}