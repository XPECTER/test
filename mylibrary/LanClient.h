#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <Pdh.h>
#include <conio.h>
#include <time.h>
#include <locale.h>
#include <Strsafe.h>
#include <process.h>
#include <WS2tcpip.h>
#include <time.h>
#include <wchar.h>

#include "APIHook.h"
#include "CrashDump.h"

#include "MemoryPool.h"
#include "MemoryPoolTLS.h"

#include "Packet.h"
#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "StreamQueue.h"

#include "Log.h"
#include "ConfigParser.h"
#include "Profiler.h"


#pragma pack(push, 1)
struct HEADER
{
	WORD wSize;
};
#pragma pack(pop)

class CLanClient
{
private:
	typedef struct st_IO_RELEASE_COMPARE
	{
		__int64 IOCount;
		__int64 ReleaseFlag;
	}IO_RELEASE_COMPARE;

	enum
	{
		dfWORKER_THREAD_NUM = 1,
		dfWSABUF_MAX = 200
	};

public:
	CLanClient();
	virtual ~CLanClient();

	bool Connect(wchar_t *szFileName);
	void Disconnect(void);

	void SendPacket(CPacket *pPacket);

protected:
	virtual void OnEnterJoinServer(void) = 0;										// �������� ���� ���� ��
	virtual void OnLeaveServer(void) = 0;											// �������� ������ �������� ��

	virtual void OnRecv(CPacket *pPacket) = 0;										// ��Ŷ(�޽���) ����, 1 Message �� 1 �̺�Ʈ.
	virtual void OnSend(int iSendSize) = 0;											// ��Ŷ(�޽���) �۽� �Ϸ� �̺�Ʈ.

	//virtual void OnWorkerThreadBegin(void) = 0;										// ��Ŀ������ 1ȸ ���� ����.
	//virtual void OnWorkerThreadEnd(void) = 0;										// ��Ŀ������ 1ȸ ���� ���� ��

	//virtual void OnError(int iErrorCode, WCHAR *szError) = 0;						// ���� 

private:
	bool SetNetworkConfig(wchar_t *szFileName);

	void SendPost(void);
	void RecvPost(bool bIncrementFlag);

	void PacketProc(void);

	void SessionRelease(void);

	static unsigned __stdcall	WorkerThread(void *lpParam);
	bool						WorkerThread_update(void);

protected:
	wchar_t _szServerGroupName[32];
private:
	SOCKET	_ClientSock;
	SOCKADDR_IN _ServerAddr;

	bool	_bConnected;
	bool	_bEnableNagle;
	wchar_t _szClientBindIP[16];
	int		_iClientBindPort;
	wchar_t _szServerIP[16];
	int		_iServerPort;

	CStreamQueue _RecvQ;
	CLockFreeQueue<CPacket *> _SendQ;

	BOOL _bSending;
	int _iSendCount;

	OVERLAPPED _RecvOverlap;
	OVERLAPPED _SendOverlap;

	IO_RELEASE_COMPARE *_IOCompare;

	HANDLE _hIOCP;
	HANDLE _hWorkerThread[dfWORKER_THREAD_NUM];
};