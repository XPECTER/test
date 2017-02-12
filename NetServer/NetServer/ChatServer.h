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
// UpdateThread �޽���
//
// OnRecv, OnClientJoin, OnClientLeave �߻��� �޽����� ��Ƽ� UpdateThread ���� �ѱ�
////////////////////////////////////////////////////////////////////////////
struct st_UPDATE_MESSAGE
{
	int			iMsgType;
	CLIENT_ID	ClientId;
	LPVOID		Debug;		// ������ ����� �뵵�� ����. ������ �̸� �������� �����.
	CPacket		*pPacket;
};


////////////////////////////////////////////////////////////////////////////
// ����
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
// Ŭ���̾�Ʈ ������ ��ü
//
//
////////////////////////////////////////////////////////////////////////////
struct st_PLAYER
{
	CLIENT_ID	ClientID;								// OnClientJoin ���� Set

	INT64		AccountNo;								// PacketProc_LoginRequest���� Set
	WCHAR		szID[dfID_MAX_LEN];						// PacketProc_LoginRequest���� Set
	WCHAR		szNick[dfNICK_MAX_LEN];					// PacketProc_LoginRequest���� Set

	BYTE		SessionKey[dfSESSION_KEY_BYTE_LEN];		// ������ ������� ����. 0����

	SECTOR_POS  Sector;

	//short		shSectorX;								// �ʱ�ȭ�� -1
	//short		shSectorY;								// �ʱ�ȭ�� -1

	bool		bDisconnectChatRecv;					// ??????
	__time64_t	LastRecvPacket;							// �ð�


	LPVOID		Debug;									// ������,, ������ ���������� ����.

};



#define		dfUPDATE_MESSAGE_JOIN			0
#define		dfUPDATE_MESSAGE_LEAVE			1
#define		dfUPDATE_MESSAGE_PACKET			3
#define		dfUPDATE_MESSAGE_HEARTBEAT		4


////////////////////////////////////////////////////////////////////////////
// ä�ü��� ��ü
//
//
////////////////////////////////////////////////////////////////////////////
class CChatServer : public CNetServer
{
public:

	CChatServer();
	virtual ~CChatServer();


protected:

	virtual void OnClientJoin(CLIENT_ID clientID);			// Client Accept �� �̺�Ʈ. map�� �ִ°� �̋�?
	virtual void OnClientLeave(CLIENT_ID clientID);							// Client Disconnect �̺�Ʈ

	//virtual bool OnConnectionRequest(CLIENT_CONNECT_INFO *pClientConnectInfo);  // Client Accept ����, ������� Ȯ�� �̺�Ʈ (return true ��� / return false �ź�)
	virtual bool OnConnectionRequest(WCHAR *szIP, int iPort);

	virtual void OnRecv(CLIENT_ID clientID, CPacket *pPacket);					// ��Ŷ(�޽���) ����, 1 Message �� 1 �̺�Ʈ.
	virtual void OnSend(CLIENT_ID clientID, int iSendSize);					// ��Ŷ(�޽���) �۽� �Ϸ� �̺�Ʈ.

	virtual void OnError(int errorNo, CLIENT_ID clientId, WCHAR *errStr);							// ���� 

	//-----------------------------------------------------------
	/////////////////////////////////////////////////////////////
	//
	// UpdateThread ä�ü��� ������ ó����
	//
	/////////////////////////////////////////////////////////////
	//-----------------------------------------------------------
	static unsigned __stdcall	UpdateThread(void *pParam);
	bool						UpdateThread_update(void);

public:
	bool Start(WCHAR *szIP, int iPort, int iThreadNum, bool bNagleEnable, int iMaxUser);

	/////////////////////////////////////////////////////////////
	//
	// ��Ŷ �� ��Ʈ��ũ �̺�Ʈ ����� UpdateThread �� �ѱ�� ���� �޽��� ����!
	//
	/////////////////////////////////////////////////////////////
	bool	EnqueueMessage(int iMsgType, CLIENT_ID clientID, CPacket *pPacket, LPVOID Debug = NULL);


	/////////////////////////////////////////////////////////////
	//
	// ������ ó�� �Լ���...
	//
	/////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////
	// �ű� ������, ������ ���� ó��.
	/////////////////////////////////////////////////////////////
	bool	JoinPlayer(CLIENT_ID clientID, LPVOID Debug);
	bool	LeavePlayer(CLIENT_ID clientID);



	/////////////////////////////////////////////////////////////
	// ��Ŷó��
	/////////////////////////////////////////////////////////////
	bool	PacketProc(st_UPDATE_MESSAGE *msg);
	bool	PacketProc_LoginRequest(st_PLAYER *pPlayer, CPacket *pPacket);
	bool	PacketProc_MoveSector(st_PLAYER *pPlayer, CPacket *pPacket);
	bool	PacketProc_ChatMessage(st_PLAYER *pPlayer, CPacket *pPacket);
	bool	PacketProc_Heartbeat(st_PLAYER *pPlayer, CPacket *pPacket);



	/////////////////////////////////////////////////////////////
	// ��Ŷ �����
	/////////////////////////////////////////////////////////////
	CPacket	*MakePacket_LoginResponse(st_PLAYER *pPlayer, BYTE Status);
	CPacket *MakePacket_MoveSector(st_PLAYER *pPlayer);
	CPacket *MakePacket_ChatMessage(st_PLAYER *pPlayer, WCHAR *szMessage, WORD iMessageSize);
	CPacket *MakePacket_HeartBeat(void);

	st_PLAYER *FindPlayer(CLIENT_ID clientID);

	/////////////////////////////////////////////////////////////
	// ���� ó��
	/////////////////////////////////////////////////////////////
	bool	EnterSector(CLIENT_ID clientID, short iSectorX, short iSectorY);
	bool	LeaveSector(CLIENT_ID clientID, short iSectorX, short iSectorY);
	bool	ValidSector(short iSectorX, short iSectorY);
	void	GetSectorAround(short iSectorX, short iSectorY, SECTOR_AROUND *pAround);

	/////////////////////////////////////////////////////////////
	// ��Ʈ��Ʈ
	/////////////////////////////////////////////////////////////
	void	SendPacket_HeartBeat(void);


	////////////////////////////////////////////////////////////////////
	// ��Ŷ ��� �Լ�.
	////////////////////////////////////////////////////////////////////
	void	SendPacket_SectorOne(short iSectorX, short iSectorY, CPacket *pPacket, CLIENT_ID ExceptClientID);
	void	SendPacket_Unicast(CLIENT_ID TargetClientID, CPacket *pPacket);
	void	SendPacket_Around(CLIENT_ID TargetClientID, CPacket *pPacket, bool bSendMe = false);
	//void	SendPacket_BroadCast(CLIENT_ID TargetClientID, CPacket *pPacket); // Ÿ�� ID�� �ʿ��� ������??
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
	// �� Ŭ�������� ���Ǵ� �����͵� �޸�Ǯ
	//-----------------------------------------------------------
	CMemoryPool<st_UPDATE_MESSAGE>	_MemoryPool_UpdateMsg;
	CMemoryPool<st_PLAYER>			_MemoryPool_Player;


protected:

	//-------------------------------------------------------------
	// �� �ڷ� ���� ����
	//-------------------------------------------------------------
	std::map<CLIENT_ID, st_PLAYER *>		_PlayerMap;
	std::map<__int64, char *>				_SessionKeyMap;


	//-------------------------------------------------------------
	// ����� ĳ���� ����
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