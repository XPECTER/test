// NetServer.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//
#pragma comment (lib, "Ws2_32.lib")

#include "stdafx.h"
#include <conio.h>
#include <locale.h>
#include "NetServer.h"
#include "ChatServer.h"
#include "../../mylibrary/ConfigParser.h"
#include "main.h"

wchar_t g_szChatServerIP[16] = { 0, };
int g_iPort = 0;
int g_iClientMax = 0;
int g_packetCode = 0;
int g_packetKey_1 = 0;
int g_packetKey_2 = 0;
int g_iThreadNum = 0;

CMemoryPoolTLS<CPacket> CPacket::PacketPool(300, true);
BYTE CPacket::_packetCode = 0;
BYTE CPacket::_packetKey_1 = 0;
BYTE CPacket::_packetKey_2 = 0;

bool g_bPrintConfig = false;
bool g_bNagle = false;



// Config파일에서 parsing해서 세팅해줘야함.

int _tmain(int argc, _TCHAR* argv[])
{
	setlocale(LC_ALL, "");
	LoadConfigAndSet();

	CChatServer chatserver;
	SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"//--------- Start ChatServer ---------//");
	chatserver.Start(g_szChatServerIP, g_iPort, g_iThreadNum, g_bNagle, g_iClientMax);

	Profiler_Init();

	DWORD keyResult = 0;

	tm t;
	time_t startTime = time(NULL);
	localtime_s(&t, &startTime);

	while (true)
	{
		keyResult = KeyProcess();

		if (1 == keyResult)
			chatserver.Stop();
		else if (2 == keyResult)
			chatserver.Start(g_szChatServerIP, g_iPort, g_iThreadNum, g_bNagle, g_iClientMax);

		Sleep(998);
		system("cls");
		wprintf_s(L"SERVER ON TIME : [%04d-%02d-%02d %02d:%02d:%02d]", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
		wprintf_s(L"\n===============================================================\n");
		
		wprintf_s(L"Session Count : %d\n", chatserver.GetClientCount());
		wprintf_s(L"Packet Pool Alloc Size : %d\n", CPacket::PacketPool.GetAllocCount());
		wprintf_s(L"Using Packet : %d\n\n", CPacket::PacketPool.GetUseCount());
		
		wprintf_s(L"Update Message Pool Alooc Size : %d\n", chatserver.GetUpdateMessagePoolAllocSize());
		wprintf_s(L"Update Message Queue Remain Size : %d\n\n", chatserver.GetUpdateMessageQueueSize());

		wprintf_s(L"Player Pool Alloc Size : %d\n", chatserver._MemoryPool_Player.GetAllocCount());
		wprintf_s(L"Player Count : %d\n\n", chatserver.GetPlayerCount());

		wprintf_s(L"Accept Total : %I64u\n", chatserver._iAcceptTotal);
		wprintf_s(L"Accept TPS : %d\n", chatserver._iAcceptTPS);
		wprintf_s(L"Update TPS : %d\n\n", chatserver._Monitor_UpdateTPS);

		wprintf_s(L"RecvPacket TPS : %d\n", chatserver._iRecvPacketTPS);
		wprintf_s(L"SendPacket TPS : %d\n\n", chatserver._iSendPacketTPS);

		PrintConfig();
	}

	return 0;
}


DWORD KeyProcess(void)
{
	int key;

	if (0 != _kbhit())
	{
		key = _getch();

		if ('Q' == key || 'q' == key)
		{
			return 1;
		}
		else if ('P' == key || 'p' == key)
		{
			Profiler_Print();
		}
		else if ('C' == key || 'c' == key)
		{
			Profiler_Clear();
		}
		else if ('S' == key || 's' == key)
		{
			return 2;
		}
		else if ('I' == key || 'i' == key)
		{
			if (true == g_bPrintConfig)
				g_bPrintConfig = false;
			else
				g_bPrintConfig = true;
		}
	}
	return 0;
}


bool LoadConfigAndSet(void)
{
	CConfigParser configParser;
	configParser.OpenConfigFile(L"ServerConfig.ini");
	
	if (!configParser.MoveConfigBlock(L"LOG"))
	{
		wprintf_s(L"ConfigParseError : LOG Block\n");
		return false;
	}
	else
	{
		configParser.GetValue(L"LOG_LEVEL", &LOG::iLogLevel);
		configParser.GetValue(L"PRINT_CONSOLE", &LOG::bConsole);
		configParser.GetValue(L"PRINT_FILE", &LOG::bFile);
		configParser.GetValue(L"PRINT_DATABASE", &LOG::bDatabase);
	}

	if (!configParser.MoveConfigBlock(L"CHAT_SERVER"))
	{
		wprintf_s(L"ConfigParseError : CHAT_SERVER Block\n");
		return false;
	}
	else
	{
		configParser.GetValue(L"BIND_IP", g_szChatServerIP, 16);
		configParser.GetValue(L"BIND_PORT", &g_iPort);
		configParser.GetValue(L"NAGLE_OPT", &g_bNagle);
		configParser.GetValue(L"THREAD_NUM", &g_iThreadNum);
	}
	
	if (!configParser.MoveConfigBlock(L"SYSTEM"))
	{
		wprintf_s(L"ConfigParseError : SYSTEM Block\n");
		return false;
	}
	else
	{
		configParser.GetValue(L"CLIENT_MAX", &g_iClientMax);
		configParser.GetValue(L"PACKET_CODE", &g_packetCode);
		configParser.GetValue(L"PACKET_KEY1", &g_packetKey_1);
		configParser.GetValue(L"PACKET_KEY2", &g_packetKey_2);
		CPacket::SetEncryptCode((BYTE)g_packetCode, (BYTE)g_packetKey_1, (BYTE)g_packetKey_2);
	}

	return true;
}

void PrintConfig(void)
{
	if (!g_bPrintConfig)
	{
		wprintf_s(L"Press [i] : See Config\n\n");
	}
	else
	{
		wprintf_s(L"Press [i] : Hide Config\n\n");
		wprintf_s(L"BIND_IP\t\t: %s\n", g_szChatServerIP);
		wprintf_s(L"BIND_PORT\t: %d\n", g_iPort);
		wprintf_s(L"NAGLE_OPT\t: %s\n", g_bNagle == true? L"ON" : L"OFF");
		wprintf_s(L"CLIENT_MAX\t: %d\n", g_iClientMax);
		wprintf_s(L"PACKET_CODE\t: 0x%x\n", g_packetCode);
		wprintf_s(L"PACKET_KEY1\t: 0x%x\n", g_packetKey_1);
		wprintf_s(L"PACKET_KEY2\t: 0x%x\n", g_packetKey_2);
	}

	return;
}