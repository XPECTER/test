#pragma once
#define dfSERVER_NAME_LEN 32
#define dfSERVER_LINK_NUM 10
#define dfSERVER_ADDR_LEN 16

#define dfDUMMY_ACCOUNTNO_MAX 999999

#define dfPLAYER_TIMEOUT_TICK 20000 
#define dfSERVER_TIMEOUT_TICK 30000
#define dfUPDATE_TICK 1000

class CLanServer_Login;

class CLoginServer : public CNetServer
{
private:
	struct st_SERVER_LINK_INFO
	{
		wchar_t _serverName[dfSERVER_NAME_LEN];

		wchar_t _gameServerIP[dfSERVER_ADDR_LEN];
		int		_gameServerPort;
		wchar_t _chatServerIP[dfSERVER_ADDR_LEN];
		int		_chatServerPort;

		wchar_t _gameServerIP_Dummy[dfSERVER_ADDR_LEN];
		int		_gameServerPort_Dummy;
		wchar_t _chatServerIP_Dummy[dfSERVER_ADDR_LEN];
		int		_chatServerPort_Dummy;
	};

	struct st_PLAYER
	{
		CLIENT_ID _clientID;

		__int64 _accountNo;
		char *_sessionKey;
		__int64 _timeoutTick;

		bool _bGameServerRecv;
		bool _bChatServerRecv;
	};

public:

	CLoginServer();
	virtual ~CLoginServer();

	bool Start(void);

protected: 
	virtual bool OnConnectionRequest(wchar_t *szClientIP, int iPort) override;   // accept ���� ��û�� �Դٴ� ���� ���������� �˷��ְ� IP Table���� ���� �źε� IP�� false. �ƴϸ� true
	
	virtual void OnClientJoin(CLIENT_ID clientID) override;						// ���������� ������ �����ߴٴ� ���� �˷���. WSARecv���� ȣ��
	virtual void OnClientLeave(CLIENT_ID clientID) override;						// ���������� ������ �����ٴ� ���� �˷���. ClientRelease���� ��� �ϴ� �������.
		   
	virtual void OnRecv(CLIENT_ID clientID, CPacket *pPacket) override;			// ��Ŷ ���� �Ϸ� ��
	virtual void OnSend(CLIENT_ID clientID, int sendsize) override;				// ��Ŷ �۽� �Ϸ� ��
	
	virtual void OnWorkerThreadBegin(void) override;									// ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
	virtual void OnWorkerThreadEnd(void) override;									// ��Ŀ������ 1���� ���� ��

	virtual void OnError(int errorNo, CLIENT_ID clientID, WCHAR *errstr) override;

	//-----------------------------------------------------------
	// �α��μ��� �����췯 ����
	//-----------------------------------------------------------
	static unsigned __stdcall	UpdateThread(void *pParam);

	void				Schedule_PlayerTimeout(void);
	void				Schedule_ServerTimeout(void);

public:
	bool	SetConfigData();
	int		GetServerNum(wchar_t *serverName);

	bool	PacketProc_NewClientLogin(CLIENT_ID clientID, int serverType);

	/////////////////////////////////////////////////////////////
	// ����� �����Լ���.  
	/////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////
	// OnClientJoin, OnClientLeave ���� ȣ���.
	/////////////////////////////////////////////////////////////
	bool				InsertPlayer(CLIENT_ID ClientID);
	bool				RemovePlayer(CLIENT_ID ClientID);

	/////////////////////////////////////////////////////////////
	// OnRecv ���� �α������� ó�� �� ���,  UpdateThread ���� �����ð� ���� ������ ���.
	/////////////////////////////////////////////////////////////
	bool				DisconnectPlayer(CLIENT_ID ClientID);

	/////////////////////////////////////////////////////////////
	// ���� ����� �� 
	/////////////////////////////////////////////////////////////
	int					GetPlayerCount(void);


	/////////////////////////////////////////////////////////////
	// White IP ����
	/////////////////////////////////////////////////////////////

	//..WhiteIP ����� �����ϱ� �˻�, Ȯ�� �� �� �ִ� ���

	/////////////////////////////////////////////////////////////
	// Lan���� ���� ������ ��Ŷ�� ���� �˰��ִ� �������� Ȯ��
	/////////////////////////////////////////////////////////////
	int				CheckServerLink(wchar_t *serverName);

private:
	st_PLAYER* FindPlayer(CLIENT_ID clientID);
	//st_PLAYER* FindPlayer(__int64 accountNo);

	void SendPacket_ResponseLogin(CLIENT_ID clientID, BYTE state);

protected:

	// �α��� ��û ��Ŷó��
	bool				PacketProc_ReqLogin(CLIENT_ID ClientID, CPacket *pPacket);

	// ��Ŷ ������
	bool				MakePacket_ResLogin(CPacket *pPacket, __int64 iAccountNo, WCHAR *szID, WCHAR *szNickname, BYTE byStatus);



protected:

	//-------------------------------------------------------------
	// ������ ����.
	// 
	// �����ڴ� �� ����Ʈ�� ������.  ������ ����ȭ�� ���� SRWLock �� ����Ѵ�.
	//-------------------------------------------------------------
	//list<CPlayer *>			_PlayerList;
	SRWLOCK				_PlayerList_srwlock;


	//-------------------------------------------------------------
	// ��Ÿ ����͸��� ����,������ �Լ�.
	//-------------------------------------------------------------
	long				_Monitor_UpdateTPS;
	long				_Monitor_LoginSuccessTPS;

	long				_Monitor_LoginWait;					// �α��� ��Ŷ ���� �� ������� ��

	long				_Monitor_LoginProcessTime_Max;		// �α��� ó�� �ð� �ִ�
	long				_Monitor_LoginProcessTime_Min;		// �α��� ó�� �ð� �ּ�
	long long			_Monitor_LoginProcessTime_Total;	// �� ��
	long long			_Monitor_LoginProcessCall_Total;	// �α��� ó�� ��û �� ��

	
private:

	long				_Monitor_UpdateCounter;
	long				_Monitor_LoginSuccessCounter;


	HANDLE				_hMonitorTPSThread;
	static unsigned __stdcall	MonitorTPSThreadFunc(void *pParam);
	bool				MonitorTPSThread_update(void);

	HANDLE				_hUpdateThread;
	static unsigned __stdcall	UpdateThreadFunc(void *pParam);
	bool				UpdateThread_update(void);

	// LanServer (connected to GameServer, ChatServer)
	CLanServer_Login	*_lanserver_Login;

	// ConfigData
	// Network
	wchar_t _szServerName[64];

	wchar_t _szBindIP[16];
	int _iBindPort;

	wchar_t _szLanBindIP[16];
	int _iLanBindPort;

	wchar_t _szMonitoringServerIP[16];
	int _iMonitoringServerPort;

	int _iWorkerThreadNum;
	bool _bNagleOpt;

	// System
	int _iClientMax;

	int _iPacketCode;
	int _iPacketKey1;
	int _iPacketKey2;

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

	// ServerLinkData
	st_SERVER_LINK_INFO _serverLinkInfo[dfSERVER_LINK_NUM];
	std::map<CLIENT_ID, st_PLAYER *> _playerMap;
	CMemoryPool<st_PLAYER> _playerPool;

	SRWLOCK _srwLock;
};
