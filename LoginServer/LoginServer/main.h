#pragma once

struct st_CONFIG
{
	// Network
	wchar_t _szServerName[64];

	wchar_t _szNetBindIP[16];
	int _iNetBindPort;

	wchar_t _szLanBindIP[16];
	int _iLanBindPort;

	wchar_t _szMonitoringServerIP[16];
	int _iMonitoringServerPort;

	int _iWorkerThreadNum;

	// System
	int _iClientMax;

	BYTE _byPacketCode;
	BYTE _byPacketKey1;
	BYTE _byPacketKey2;

	int _iLogLevel;

	wchar_t _szServerLinkConfigFileName[128];

	// Database
	// AccountDB
	wchar_t _szAccountIP[16];
	int _iAccountPort;
	wchar_t _szAccountUser[32];
	wchar_t _szAccountPassword[32];
	wchar_t _szAccountDBName[32];

	// Login Log DB
	wchar_t _szLogIP[16];
	int _iLogPort;
	wchar_t _szLogUser[32];
	wchar_t _szLogPassword[32];
	wchar_t _szLogDBName[32];
};

bool Init_main(void);

extern CCrashDump crashDump;