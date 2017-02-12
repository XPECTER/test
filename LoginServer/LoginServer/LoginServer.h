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
	virtual bool OnConnectionRequest(wchar_t *szClientIP, int iPort) override;   // accept 직후 요청이 왔다는 것을 컨텐츠에게 알려주고 IP Table에서 접속 거부된 IP면 false. 아니면 true
	
	virtual void OnClientJoin(CLIENT_ID clientID) override;						// 컨텐츠에게 유저가 접속했다는 것을 알려줌. WSARecv전에 호출
	virtual void OnClientLeave(CLIENT_ID clientID) override;						// 컨텐츠에게 유저가 떠났다는 것을 알려줌. ClientRelease에서 상단 하단 상관없음.
		   
	virtual void OnRecv(CLIENT_ID clientID, CPacket *pPacket) override;			// 패킷 수신 완료 후
	virtual void OnSend(CLIENT_ID clientID, int sendsize) override;				// 패킷 송신 완료 후
	
	virtual void OnWorkerThreadBegin(void) override;									// 워커스레드 GQCS 바로 하단에서 호출
	virtual void OnWorkerThreadEnd(void) override;									// 워커스레드 1루프 종료 후

	virtual void OnError(int errorNo, CLIENT_ID clientID, WCHAR *errstr) override;

	//-----------------------------------------------------------
	// 로그인서버 스케쥴러 역할
	//-----------------------------------------------------------
	static unsigned __stdcall	UpdateThread(void *pParam);

	void				Schedule_PlayerTimeout(void);
	void				Schedule_ServerTimeout(void);

public:
	bool	SetConfigData();
	int		GetServerNum(wchar_t *serverName);

	bool	PacketProc_NewClientLogin(CLIENT_ID clientID, int serverType);

	/////////////////////////////////////////////////////////////
	// 사용자 관리함수들.  
	/////////////////////////////////////////////////////////////

	/////////////////////////////////////////////////////////////
	// OnClientJoin, OnClientLeave 에서 호출됨.
	/////////////////////////////////////////////////////////////
	bool				InsertPlayer(CLIENT_ID ClientID);
	bool				RemovePlayer(CLIENT_ID ClientID);

	/////////////////////////////////////////////////////////////
	// OnRecv 에서 로그인인증 처리 후 사용,  UpdateThread 에서 일정시간 지난 유저에 사용.
	/////////////////////////////////////////////////////////////
	bool				DisconnectPlayer(CLIENT_ID ClientID);

	/////////////////////////////////////////////////////////////
	// 접속 사용자 수 
	/////////////////////////////////////////////////////////////
	int					GetPlayerCount(void);


	/////////////////////////////////////////////////////////////
	// White IP 관련
	/////////////////////////////////////////////////////////////

	//..WhiteIP 목록을 저장하구 검색, 확인 할 수 있는 기능

	/////////////////////////////////////////////////////////////
	// Lan으로 받은 서버의 패킷이 내가 알고있는 서버인지 확인
	/////////////////////////////////////////////////////////////
	int				CheckServerLink(wchar_t *serverName);

private:
	st_PLAYER* FindPlayer(CLIENT_ID clientID);
	//st_PLAYER* FindPlayer(__int64 accountNo);

	void SendPacket_ResponseLogin(CLIENT_ID clientID, BYTE state);

protected:

	// 로그인 요청 패킷처리
	bool				PacketProc_ReqLogin(CLIENT_ID ClientID, CPacket *pPacket);

	// 패킷 생성부
	bool				MakePacket_ResLogin(CPacket *pPacket, __int64 iAccountNo, WCHAR *szID, WCHAR *szNickname, BYTE byStatus);



protected:

	//-------------------------------------------------------------
	// 접속자 관리.
	// 
	// 접속자는 본 리스트로 관리함.  스레드 동기화를 위해 SRWLock 을 사용한다.
	//-------------------------------------------------------------
	//list<CPlayer *>			_PlayerList;
	SRWLOCK				_PlayerList_srwlock;


	//-------------------------------------------------------------
	// 기타 모니터링용 변수,스레드 함수.
	//-------------------------------------------------------------
	long				_Monitor_UpdateTPS;
	long				_Monitor_LoginSuccessTPS;

	long				_Monitor_LoginWait;					// 로그인 패킷 수신 후 대기중인 수

	long				_Monitor_LoginProcessTime_Max;		// 로그인 처리 시간 최대
	long				_Monitor_LoginProcessTime_Min;		// 로그인 처리 시간 최소
	long long			_Monitor_LoginProcessTime_Total;	// 총 합
	long long			_Monitor_LoginProcessCall_Total;	// 로그인 처리 요청 총 합

	
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
