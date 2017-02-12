#pragma once
#include <WS2tcpip.h>
#include <process.h>
#include <time.h>
#include <mstcpip.h>
#include <stdio.h>

#include "MemoryPool.h"
#include "MemoryPoolTLS.h"
#include "LockFreeQueue.h"
#include "LockFreeStack.h"
#include "StreamQueue.h"
#include "Packet.h"
#include "Profiler.h"
#include "Log.h"

#define MAKECLIENTID(index, id) (((__int64)index << 48) | id)
#define EXTRACTCLIENTINDEX(ClientID) ((__int64)ClientID >> 48)
#define EXTRACTCLIENTID(ClientID) ((__int64)ClientID & 0x00FFFFFF)

typedef __int64 CLIENT_ID;

//typedef struct st_HEADER
//{
//	WORD wSize;
//}HEADER;

class CNetServer
{
private:
	enum en_DEFINE
	{
		dfMIN_THREAD_COUNT = 3,
		dfMAX_THREAD_COUNT = 50,
		dfPACKET_CODE = 0x78
	};

	typedef struct st_IO_RELEASE_COMPARE
	{
		LONG64 IOCount;
		LONG64 ReleaseFlag;
	}IO_RELEASE_COMPARE;

	typedef struct st_SESSION
	{
		CLIENT_ID					ClientId;
		SOCKET						ClientSock;
		SOCKADDR_IN					ClientAddr;

		CStreamQueue				RecvQ;
		CLockFreeQueue<CPacket *>	SendQ;

		OVERLAPPED					RecvOverlap;
		OVERLAPPED					SendOverlap;

		BOOL						Sending;
		IO_RELEASE_COMPARE			*IOReleaseCompare;
		int							SendCount;
	}SESSION;

public:
	CNetServer();
	~CNetServer();

	bool Start(WCHAR *szIP, int iPort, int iThreadNum, bool bNagleEnable, int iMaxUser);
	void Stop();

	void SendPacket(CLIENT_ID clientId, CPacket *pPacket);
	int GetClientCount(void);

	void ShutDown(CLIENT_ID _clientId);										// shutdown(s, SD_SEND)
	void ClientDisconnect(CLIENT_ID _clientId);								// shutdown(s, SD_BOTH)


	// 이벤트 용도로 사용하는 함수
protected:
	virtual bool OnConnectionRequest(WCHAR *_szClientIP, int _iPort) = 0;   // accept 직후 요청이 왔다는 것을 컨텐츠에게 알려주고 IP Table에서 접속 거부된 IP면 false. 아니면 true

	virtual void OnClientJoin(CLIENT_ID _clientId) = 0;						// 컨텐츠에게 유저가 접속했다는 것을 알려줌. WSARecv전에 호출
	virtual void OnClientLeave(CLIENT_ID _clientId) = 0;						// 컨텐츠에게 유저가 떠났다는 것을 알려줌. ClientRelease에서 상단 하단 상관없음.

	virtual void OnRecv(CLIENT_ID _clientId, CPacket *pPacket) = 0;			// 패킷 수신 완료 후
	virtual void OnSend(CLIENT_ID _clientId, int sendsize) = 0;				// 패킷 송신 완료 후

	virtual void OnWorkerThreadBegin() = 0;									// 워커스레드 GQCS 바로 하단에서 호출
	virtual void OnWorkerThreadEnd() = 0;									// 워커스레드 1루프 종료 후

	virtual void OnError(int _errorNo, CLIENT_ID _clientId, WCHAR *_errstr) = 0;


private:
	void Init_Session(int _iMaxUser);										// 세션 초기화
	bool Init_Thread(int _iThreadNum);										// 스레드 생성

	// 데이터 처리
	bool PacketProc(SESSION *pSession);
	void SendPost(SESSION *pSession);
	void RecvPost(SESSION *pSession, bool incrementFlag);

	SESSION* AcquireSessionLock(CLIENT_ID clientId); // 세션 잠금
	void ReleaseSessionLock(CLIENT_ID clientId);		// 세션 잠금 해제

	void ShtuDown(SESSION *pSession);				// shutdown(s, SD_SEND)
	void ClientDisconnect(SESSION *pSession);		// shutdown(s, SD_BOTH)
	void ClientRelease(SESSION *pSession);			// Session 해제

	static unsigned _stdcall AcceptThreadFunc(void *lpParam);
	static unsigned _stdcall WorkerThreadFunc(void *lpParam);
	static unsigned _stdcall MonitorThreadFunc(void *lpParam);

	// 모니터링 용도
public:
	int _iAcceptTPS;
	int _iRecvPacketTPS;
	int _iSendPacketTPS;
	unsigned __int64 _iAcceptTotal;

	// Log 
protected:
	FILE *_SystemErrorLogFile;

	// 멤버 변수
private:
	bool _bWorking;
	bool _bStop;

	HANDLE _hIOCP;
	HANDLE _hAcceptThread;
	HANDLE *_hWorkerThread;
	HANDLE _hMonitorThread;

	int _iWorkerThreadNum;
	int _iMaxUser;
	unsigned __int64 _iClientID;
	long _iClientCount;
	time_t _timeStamp;

	long _iAcceptCounter;
	long _iRecvPacketCounter;
	long _iSendPacketCounter;

	SESSION *_aSession;
	SOCKET _listenSock;
	SOCKADDR_IN _serverAddr;
	CLockFreeStack<int> _indexStack;
};


