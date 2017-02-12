#pragma once

#define dfSERVER_GROUP_NAME_LEN 128
#define dfGAME_SERVER_NAME_LEN 128
#define dfCHAT_SERVER_NAME_LEN 128



struct st_CONFIG
{
	wchar_t szServerGroupName[dfSERVER_GROUP_NAME_LEN];

	wchar_t szBindIP[16];
	int iBindPort;

	wchar_t szMonitoringServerIP[16];
	int iMonitoringServerPort;

	wchar_t szGameServerFileName[dfGAME_SERVER_NAME_LEN];
	wchar_t szChatServerFileName[dfCHAT_SERVER_NAME_LEN];
};

bool Init_main();
void KeyProcess();
void PrintState();

extern st_CONFIG g_ConfigData;
extern CCrashDump crashDump;
extern CAgent g_Agent;