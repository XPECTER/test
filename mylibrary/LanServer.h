#pragma once
#pragma comment(lib, "Ws2_32.lib")
#include <process.h>
#include <time.h>
#include <stdio.h>
#include <tchar.h>
#include <WinSock2.h>
#include <mstcpip.h>
#include <WS2tcpip.h>
#include <signal.h>
#include <crtdbg.h>
#include <Psapi.h>
#include <stdio.h>
#include <DbgHelp.h>
#include <TlHelp32.h>

#include "APIHook.h"
#include "CrashDump.h"

#include "MemoryPool.h"
#include "MemoryPoolTLS.h"
#include "Packet.h"

#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "StreamQueue.h"

#include "ConfigParser.h"
#include "Log.h"
#include "Profiler.h"

typedef __int64 ClientID;

#pragma pack(push, 1)
struct st_HEADER
{
	WORD _wSize;
};
#pragma pack(pop)

class CLanServer
{
public:
	enum define
	{
		dfMIN_THREAD_COUNT = 3,
		dfMAX_THREAD_COUNT = 50
	};

	typedef struct st_IO_RELEASE_COMPARE
	{
		LONG64 IOCount;
		LONG64 bReleaseFlag;
	}IO_RELEASE_COMPARE;

	typedef struct st_SESSION
	{
		int				_iIndex;

		ClientID		ClientId;
		SOCKET			ClientSock;
		SOCKADDR_IN		ClientAddr;

		CStreamQueue	RecvQ;
		CLockFreeQueue<CPacket *>	SendQ;
		
		OVERLAPPED			RecvOverlap;
		OVERLAPPED			SendOverlap;
		
		BOOL				_bSending;
		IO_RELEASE_COMPARE	*IOReleaseCompare;
		int					_sendCount;
	}SESSION;

public :
	CLanServer();
	~CLanServer();

	bool Start(WCHAR *_ip, int _port, int _iThreadNum, bool _bNagleEnable, int _iMaxUser);
	void Stop();
	void SendPacket(ClientID _clientId, CPacket *buff);

	int GetClientCount(void);


// 데이터 처리
private:
	int PacketProc(SESSION *pSession);

	void SendPost(SESSION *pSession);
	void RecvPost(SESSION *pSession, bool incrementFlag);


// 세션 잠금 기능 관련 함수
private:
	SESSION* AcquireSessionLock(ClientID clientId);
	void ReleaseSessionLock(ClientID clientId);


// 접속 해제 관련 함수
public:
	void ShutDown(ClientID _clientId);				// shutdown(s, SD_SEND)
	void ClientDisconnect(ClientID _clientId);		// shutdown(s, SD_BOTH)

private:
	void ShtuDown(SESSION *pSession);				// shutdown(s, SD_SEND)
	void ClientDisconnect(SESSION *pSession);		// shutdown(s, SD_BOTH)
	void ClientRelease(SESSION *pSession);			// Session 해제


// 이벤트 용도로 사용하는 함수
protected:
	virtual bool OnConnectionRequest(wchar_t *szClientIP, int iPort) = 0;   // accept 직후 요청이 왔다는 것을 컨텐츠에게 알려주고 IP Table에서 접속 거부된 IP면 false. 아니면 true

	virtual void OnClientJoin(ClientID clientID) = 0;						// 컨텐츠에게 유저가 접속했다는 것을 알려줌. WSARecv전에 호출
	virtual void OnClientLeave(ClientID clientID) = 0;						// 컨텐츠에게 유저가 떠났다는 것을 알려줌. ClientRelease에서 상단 하단 상관없음.
	
	virtual void OnRecv(ClientID clientID, CPacket *pRecvPacket) = 0;		// 패킷 수신 완료 후
	virtual void OnSend(ClientID clientID, int sendSize) = 0;				// 패킷 송신 완료 후

	virtual void OnWorkerThreadBegin(void) = 0;									// 워커스레드 GQCS 바로 하단에서 호출
	virtual void OnWorkerThreadEnd(void) = 0;									// 워커스레드 1루프 종료 후

	virtual void OnError(int _errorNo, ClientID clientID, WCHAR *_errstr) = 0;
	
private :
	void Init_Session(int _iMaxUser);							// 세션 초기화
	BOOL Init_Thread(int _iThreadNum);							// 스레드 생성

	HANDLE hAcceptThread;
	static unsigned _stdcall AcceptThreadFunc(void *lpParam);

	HANDLE *hWorkerThread;
	static unsigned _stdcall WorkerThreadFunc(void *lpParam);
	
	HANDLE hMonitorThread;
	static unsigned _stdcall MonitorThreadFunc(void *lpParam);
	
	// 나중에 도입
	//HANDLE hSendThread;
	//static unsigned _stdcall SendThreadFunc(void *lpParam);

// 모니터링 용도
public:
	int _iAcceptTPS;
	int _iRecvPacketTPS;
	int _iSendPacketTPS;

// Log 
protected:
	FILE *SystemErrorLogFile;
	SRWLOCK srwLock;

// 멤버 변수
private :
	bool _bWorking;
	bool _bStop;

	SESSION *aSession;

	SOCKET _listenSock;
	HANDLE hIOCP;
	int _iWorkerThreadNum;
	int _iMaxUser;
	__int64 _iClientID;
	int _iClientCount;
	int _iTPS;
	time_t _timeStamp;

	int _iAcceptCounter;
	int _iRecvPacketCounter;
	int _iSendPacketCounter;

	SOCKADDR_IN _serverAddr;
	CLockFreeStack<int> _indexStack;
};