// LoginServer.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "LoginServer.h"
#include "LanServer_Login.h"
#include "main.h"

CMemoryPoolTLS<CPacket> CPacket::PacketPool(300, true);
BYTE CPacket::_packetCode = 0;
BYTE CPacket::_packetKey_1 = 0;
BYTE CPacket::_packetKey_2 = 0;

long CCrashDump::_DumpCount = 0;
CCrashDump crashDump;
st_CONFIG g_ConfigData;

CLoginServer loginServer;

int _tmain(int argc, _TCHAR* argv[])
{
	//loginServer.SetConfigData();
	loginServer.Start();

	while (true)
	{
		Sleep(998);
		wprintf_s(L"AcceptTotal : %I64u\n", loginServer._iAcceptTotal);
	}

	return 0;
}

