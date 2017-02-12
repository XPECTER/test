#pragma once
#include "CommonProtocol.h"
#include "NetServer.h"

#define dfID_MAX_LEN 20
#define dfNICK_MAX_LEN 20
#define dfSESSION_KEY_BYTE_LEN 64

#define dfSECTOR_X_DEFAULT -1
#define dfSECTOR_Y_DEFAULT -1
#define dfSECTOR_X_MAX 50
#define dfSECTOR_Y_MAX 50



////////////////////////////////////////////////////////////////////////////
// UpdateThread 메시지
//
// OnRecv, OnClientJoin, OnClientLeave 발생시 메시지로 담아서 UpdateThread 에게 넘김
////////////////////////////////////////////////////////////////////////////
struct st_UPDATE_MESSAGE
{
	int			iMsgType;
	CLIENT_ID	ClientId;
	LPVOID		Debug;		// 임의의 디버깅 용도의 정보. 지금은 이를 소켓으로 사용중.
	CPacket		*pPacket;
};


////////////////////////////////////////////////////////////////////////////
// 섹터
//
//
////////////////////////////////////////////////////////////////////////////
typedef struct st_SECTOR_POS
{
	short shSectorX;
	short shSectorY;
}SECTOR_POS;

typedef struct st_SECTOR_AROUND
{
	int iCount;
	SECTOR_POS Around[9];

	st_SECTOR_AROUND()
	{
		iCount = 0;
		memset(Around, 0, sizeof(SECTOR_POS) * 9);
	}
}SECTOR_AROUND;


////////////////////////////////////////////////////////////////////////////
// 클라이언트 컨텐츠 객체
//
//
////////////////////////////////////////////////////////////////////////////
struct st_PLAYER
{
	CLIENT_ID	ClientID;								// OnClientJoin 에서 Set

	INT64		AccountNo;								// PacketProc_LoginRequest에서 Set
	WCHAR		szID[dfID_MAX_LEN];						// PacketProc_LoginRequest에서 Set
	WCHAR		szNick[dfNICK_MAX_LEN];					// PacketProc_LoginRequest에서 Set

	BYTE		SessionKey[dfSESSION_KEY_BYTE_LEN];		// 지금은 사용하지 않음. 0으로

	SECTOR_POS  Sector;

	//short		shSectorX;								// 초기화시 -1
	//short		shSectorY;								// 초기화시 -1

	bool		bDisconnectChatRecv;					// ??????
	__time64_t	LastRecvPacket;							// 시간


	LPVOID		Debug;									// 디버깅용,, 지금은 소켓정보를 담음.

};



#define		dfUPDATE_MESSAGE_JOIN			0
#define		dfUPDATE_MESSAGE_LEAVE			1
#define		dfUPDATE_MESSAGE_PACKET			3
#define		dfUPDATE_MESSAGE_HEARTBEAT		4


////////////////////////////////////////////////////////////////////////////
// 채팅서버 본체
//
//
////////////////////////////////////////////////////////////////////////////
class CChatServer : public CNetServer
{
public:

	CChatServer();
	virtual ~CChatServer();


protected:

	virtual void OnClientJoin(CLIENT_ID clientID);			// Client Accept 후 이벤트. map에 넣는건 이떄?
	virtual void OnClientLeave(CLIENT_ID clientID);							// Client Disconnect 이벤트

	//virtual bool OnConnectionRequest(CLIENT_CONNECT_INFO *pClientConnectInfo);  // Client Accept 직후, 연결허용 확인 이벤트 (return true 허용 / return false 거부)
	virtual bool OnConnectionRequest(WCHAR *szIP, int iPort);

	virtual void OnRecv(CLIENT_ID clientID, CPacket *pPacket);					// 패킷(메시지) 수신, 1 Message 당 1 이벤트.
	virtual void OnSend(CLIENT_ID clientID, int iSendSize);					// 패킷(메시지) 송신 완료 이벤트.

	virtual void OnError(int errorNo, CLIENT_ID clientId, WCHAR *errStr);							// 에러 

	//-----------------------------------------------------------
	/////////////////////////////////////////////////////////////
	//
	// UpdateThread 채팅서버 컨텐츠 처리용
	//
	/////////////////////////////////////////////////////////////
	//-----------------------------------------------------------
	static unsigned __stdcall	UpdateThread(void *pParam);
	bool						UpdateThread_update(void);

public:
	bool Start(WCHAR *szIP, int iPort, int iThreadNum, bool bNagleEnable, int iMaxUser);

	/////////////////////////////////////////////////////////////
	//
	// 패킷 및 네트워크 이벤트 결과를 UpdateThread 로 넘기기 위한 메시지 삽입!
	//
	/////////////////////////////////////////////////////////////
	bool	EnqueueMessage(int iMsgType, CLIENT_ID clientID, CPacket *pPacket, LPVOID Debug = NULL);


	/////////////////////////////////////////////////////////////
	//
	// 컨텐츠 처리 함수들...
	//
	/////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////
	// 신규 접속자, 접속자 해지 처리.
	/////////////////////////////////////////////////////////////
	bool	JoinPlayer(CLIENT_ID clientID, LPVOID Debug);
	bool	LeavePlayer(CLIENT_ID clientID);



	/////////////////////////////////////////////////////////////
	// 패킷처리
	/////////////////////////////////////////////////////////////
	bool	PacketProc(st_UPDATE_MESSAGE *msg);
	bool	PacketProc_LoginRequest(st_PLAYER *pPlayer, CPacket *pPacket);
	bool	PacketProc_MoveSector(st_PLAYER *pPlayer, CPacket *pPacket);
	bool	PacketProc_ChatMessage(st_PLAYER *pPlayer, CPacket *pPacket);
	bool	PacketProc_Heartbeat(st_PLAYER *pPlayer, CPacket *pPacket);



	/////////////////////////////////////////////////////////////
	// 패킷 만들기
	/////////////////////////////////////////////////////////////
	CPacket	*MakePacket_LoginResponse(st_PLAYER *pPlayer, BYTE Status);
	CPacket *MakePacket_MoveSector(st_PLAYER *pPlayer);
	CPacket *MakePacket_ChatMessage(st_PLAYER *pPlayer, WCHAR *szMessage, WORD iMessageSize);
	CPacket *MakePacket_HeartBeat(void);

	st_PLAYER *FindPlayer(CLIENT_ID clientID);

	/////////////////////////////////////////////////////////////
	// 섹터 처리
	/////////////////////////////////////////////////////////////
	bool	EnterSector(CLIENT_ID clientID, short iSectorX, short iSectorY);
	bool	LeaveSector(CLIENT_ID clientID, short iSectorX, short iSectorY);
	bool	ValidSector(short iSectorX, short iSectorY);
	void	GetSectorAround(short iSectorX, short iSectorY, SECTOR_AROUND *pAround);

	/////////////////////////////////////////////////////////////
	// 하트비트
	/////////////////////////////////////////////////////////////
	void	SendPacket_HeartBeat(void);


	////////////////////////////////////////////////////////////////////
	// 패킷 쏘기 함수.
	////////////////////////////////////////////////////////////////////
	void	SendPacket_SectorOne(short iSectorX, short iSectorY, CPacket *pPacket, CLIENT_ID ExceptClientID);
	void	SendPacket_Unicast(CLIENT_ID TargetClientID, CPacket *pPacket);
	void	SendPacket_Around(CLIENT_ID TargetClientID, CPacket *pPacket, bool bSendMe = false);
	//void	SendPacket_BroadCast(CLIENT_ID TargetClientID, CPacket *pPacket); // 타켓 ID가 필요한 이유는??
	void	SendPacket_Broadcast(CPacket *pPacket);

	int		GetPlayerCount(void) { return (int)_PlayerMap.size(); }
	int		GetUpdateMessageQueueSize(void)		{ return (int)_UpdateMessageQueue.GetUseSize(); }
	int		GetUpdateMessagePoolAllocSize(void) { return (int)_MemoryPool_UpdateMsg.GetAllocCount(); }
	int		GetUpdateMessagePoolUseSize(void)	{ return (int)_MemoryPool_UpdateMsg.GetUseCount(); }

	void	OnWorkerThreadBegin();
	void	OnWorkerThreadEnd();

protected:

	bool			_bShutdown;


	//-----------------------------------------------------------
	// UpdateThread Event
	//-----------------------------------------------------------
	HANDLE			_hUpdateEvent;
	HANDLE			_hUpdateThread;

public:
	//-----------------------------------------------------------
	// 본 클래스에서 사용되는 데이터들 메모리풀
	//-----------------------------------------------------------
	CMemoryPool<st_UPDATE_MESSAGE>	_MemoryPool_UpdateMsg;
	CMemoryPool<st_PLAYER>			_MemoryPool_Player;


protected:

	//-------------------------------------------------------------
	// 각 자료 메인 관리
	//-------------------------------------------------------------
	std::map<CLIENT_ID, st_PLAYER *>		_PlayerMap;
	std::map<__int64, char *>				_SessionKeyMap;


	//-------------------------------------------------------------
	// 월드맵 캐릭터 섹터
	//-------------------------------------------------------------
	std::list<CLIENT_ID>					_Sector[dfSECTOR_Y_MAX][dfSECTOR_X_MAX];


public:

	CLockFreeQueue<st_UPDATE_MESSAGE *>	_UpdateMessageQueue;

	long								_Monitor_UpdateTPS;

private:
	CLanClient_Chat *_lanclient_Chat;


	long								_Monitor_UpdateTPS_Counter;

	HANDLE								_hMonitorThread;
	static unsigned __stdcall			MonitorTPS_Thread(void *pParam);
	bool								MonitorTPS_Thread_update(void);
};