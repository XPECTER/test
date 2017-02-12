#pragma once


class CLoginServer;

class CLanServer_Login : public CLanServer
{
public:
	struct st_SERVER_SESSION
	{
		CLIENT_ID _clientID;
		BYTE _serverType;
		wchar_t _serverName[dfSERVER_NAME_LEN];
		__int64 _timeoutTick;
	};

private:
	struct st_SERVER_SESSION_SET
	{
		int _serverNum;

		CLIENT_ID _gameServerID;
		CLIENT_ID _chatServerID;
	};

public:
	CLanServer_Login(CLoginServer *pLoginServer);
	virtual ~CLanServer_Login();

	virtual bool OnConnectionRequest(wchar_t *szClientIP, int iPort) override;   // accept 직후 요청이 왔다는 것을 컨텐츠에게 알려주고 IP Table에서 접속 거부된 IP면 false. 아니면 true

	virtual void OnClientJoin(ClientID clientID) override;						// 컨텐츠에게 유저가 접속했다는 것을 알려줌. WSARecv전에 호출
	virtual void OnClientLeave(ClientID clientID) override;						// 컨텐츠에게 유저가 떠났다는 것을 알려줌. ClientRelease에서 상단 하단 상관없음.

	virtual void OnRecv(ClientID clientID, CPacket *pRecvPacket) override;	// 패킷 수신 완료 후
	virtual void OnSend(ClientID clientID, int sendSize) override;				// 패킷 송신 완료 후

	virtual void OnWorkerThreadBegin(void) override;									// 워커스레드 GQCS 바로 하단에서 호출
	virtual void OnWorkerThreadEnd(void) override;									// 워커스레드 1루프 종료 후

	virtual void OnError(int errorNo, ClientID clientID, wchar_t *errstr) override;

	void SendPacket_ServerGroup(int serverNum, CPacket *pSendPacket);
private:
	st_SERVER_SESSION* FindSession(CLIENT_ID clientID);
	st_SERVER_SESSION_SET* FindSessionSet(int serverNum);

	void PacketProc_LoginServerLogin(CLIENT_ID clientID, CPacket *pRecvPacket);
	void PacketProc_NewClientLogin(CLIENT_ID clientID, CPacket *pRecvPacket);

private:
	std::map<CLIENT_ID, st_SERVER_SESSION *> _sessionMap;
	std::list<st_SERVER_SESSION_SET *> _sessionSetList;
	SRWLOCK _sessionMapLock;
	SRWLOCK _sessionSetListLock;
	CLoginServer *_pLoginServer;
	
	HANDLE _hTimeoutCheckThread;
	unsigned __stdcall TimeoutCheckThreadFunc(void *lpParam);
	bool TimeoutCheck_update(void);
};