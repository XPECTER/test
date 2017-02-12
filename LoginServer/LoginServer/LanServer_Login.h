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

	virtual bool OnConnectionRequest(wchar_t *szClientIP, int iPort) override;   // accept ���� ��û�� �Դٴ� ���� ���������� �˷��ְ� IP Table���� ���� �źε� IP�� false. �ƴϸ� true

	virtual void OnClientJoin(ClientID clientID) override;						// ���������� ������ �����ߴٴ� ���� �˷���. WSARecv���� ȣ��
	virtual void OnClientLeave(ClientID clientID) override;						// ���������� ������ �����ٴ� ���� �˷���. ClientRelease���� ��� �ϴ� �������.

	virtual void OnRecv(ClientID clientID, CPacket *pRecvPacket) override;	// ��Ŷ ���� �Ϸ� ��
	virtual void OnSend(ClientID clientID, int sendSize) override;				// ��Ŷ �۽� �Ϸ� ��

	virtual void OnWorkerThreadBegin(void) override;									// ��Ŀ������ GQCS �ٷ� �ϴܿ��� ȣ��
	virtual void OnWorkerThreadEnd(void) override;									// ��Ŀ������ 1���� ���� ��

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