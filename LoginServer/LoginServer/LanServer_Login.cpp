#include "stdafx.h"
#include "CommonProtocol.h"
#include "LoginServer.h"
#include "LanServer_Login.h"


CLanServer_Login::CLanServer_Login(CLoginServer *pLoginServer)
{
	_hTimeoutCheckThread = INVALID_HANDLE_VALUE;
	InitializeSRWLock(&this->_sessionMapLock);
	InitializeSRWLock(&this->_sessionSetListLock);

	this->_pLoginServer = pLoginServer;
	
}

CLanServer_Login::~CLanServer_Login()
{

}

bool CLanServer_Login::OnConnectionRequest(wchar_t *szClientIP, int iPort)
{
	return true;
}

void CLanServer_Login::OnClientJoin(ClientID clientID)
{
	st_SERVER_SESSION *newSession = new st_SERVER_SESSION;
	newSession->_clientID = clientID;
	newSession->_timeoutTick = time(NULL);

	AcquireSRWLockExclusive(&this->_sessionMapLock);
	this->_sessionMap.insert(std::pair<CLIENT_ID, st_SERVER_SESSION*>(newSession->_clientID, newSession));
	ReleaseSRWLockExclusive(&this->_sessionMapLock);
	return;
}

void CLanServer_Login::OnClientLeave(ClientID clientID)
{
	st_SERVER_SESSION *pSession = this->FindSession(clientID);

	if (nullptr == pSession)
	{
		SYSLOG(L"ERROR", LOG::LEVEL_ERROR, L"Found Session is wrong");
		return;
	}
	else
	{
		AcquireSRWLockExclusive(&this->_sessionMapLock);
		delete pSession;
		this->_sessionMap.erase(clientID);
		ReleaseSRWLockExclusive(&this->_sessionMapLock);
		return;
	}
}

void CLanServer_Login::OnRecv(ClientID clientID, CPacket *pRecvPacket)
{
	WORD type;
	*pRecvPacket >> type;

	switch (type)
	{
		case en_PACKET_SS_LOGINSERVER_LOGIN:
			PacketProc_LoginServerLogin(clientID, pRecvPacket);
			break;

		case en_PACKET_SS_RES_NEW_CLIENT_LOGIN:
			break;

		case en_PACKET_SS_HEARTBEAT:
			break;

		default:
			break;
	}

	return;
}

void CLanServer_Login::OnSend(ClientID clientID, int sendSize)
{
	return;
}

void CLanServer_Login::OnWorkerThreadBegin(void)
{
	return;
}

void CLanServer_Login::OnWorkerThreadEnd(void)
{
	return;
}

void CLanServer_Login::OnError(int errorNo, ClientID clientID, wchar_t *errstr)
{
	return;
}

CLanServer_Login::st_SERVER_SESSION* CLanServer_Login::FindSession(CLIENT_ID clientID)
{
	st_SERVER_SESSION *pSession = nullptr;

	AcquireSRWLockShared(&this->_sessionMapLock);
	auto iter = this->_sessionMap.find(clientID);
	if (iter != this->_sessionMap.end())
		pSession =  iter->second;

	ReleaseSRWLockShared(&this->_sessionMapLock);
	return pSession;
}

CLanServer_Login::st_SERVER_SESSION_SET* CLanServer_Login::FindSessionSet(int serverNum)
{
	st_SERVER_SESSION_SET *pSessionSet = nullptr;

	AcquireSRWLockShared(&this->_sessionSetListLock);
	for (auto iter = this->_sessionSetList.begin(); iter != this->_sessionSetList.end();)
	{
		if (serverNum == (*iter)->_serverNum)
		{
			pSessionSet = (*iter);
			break;
		}
	}

	ReleaseSRWLockShared(&this->_sessionSetListLock);
	return pSessionSet;
}

void CLanServer_Login::SendPacket_ServerGroup(int serverNum, CPacket *pSendPacket)
{
	st_SERVER_SESSION_SET *pSessionSet = FindSessionSet(serverNum);

	if (nullptr != pSessionSet)
	{
		SendPacket(pSessionSet->_chatServerID, pSendPacket);
		SendPacket(pSessionSet->_chatServerID, pSendPacket);
	}
	
	return;
}

void CLanServer_Login::PacketProc_LoginServerLogin(CLIENT_ID clientID, CPacket *pRecvPacket)
{
	BYTE serverType;
	wchar_t serverName[32] = { 0, };

	*pRecvPacket >> serverType;
	pRecvPacket->Dequeue((char *)serverName, sizeof(serverName));

	int serverNum = this->_pLoginServer->CheckServerLink(serverName);
	if (0 == serverNum)
	{
		ClientDisconnect(clientID);
		return;
	}
	else
	{
		st_SERVER_SESSION *pSession = this->FindSession(clientID);

		if (nullptr == pSession)
		{
			SYSLOG(L"ERROR", LOG::LEVEL_ERROR, L"Found Session is wrong");
			return;
		}
		else
		{
			pSession->_serverType = serverType;
			wcscpy_s(pSession->_serverName, dfSERVER_NAME_LEN, serverName);

			st_SERVER_SESSION_SET *pSessionSet = this->FindSessionSet(serverNum);
			if (nullptr == pSessionSet)
			{
				pSessionSet = new st_SERVER_SESSION_SET;
				pSessionSet->_serverNum = serverNum;	

				if (dfSERVER_TYPE_GAME == serverType)
					pSessionSet->_gameServerID = clientID;
				else if (dfSERVER_TYPE_CHAT == serverType)
					pSessionSet->_chatServerID = clientID;

				AcquireSRWLockExclusive(&this->_sessionSetListLock);
				this->_sessionSetList.push_back(pSessionSet);
				ReleaseSRWLockExclusive(&this->_sessionSetListLock);
			}
			else
			{
				if (dfSERVER_TYPE_GAME == serverType)
					pSessionSet->_gameServerID = clientID;
				else if (dfSERVER_TYPE_CHAT == serverType)
					pSessionSet->_chatServerID = clientID;
			}

			return;
		}
	}
}

void CLanServer_Login::PacketProc_NewClientLogin(CLIENT_ID clientID, CPacket *pRecvPacket)
{
	__int64 accountNo;
	__int64 parameter;

	*pRecvPacket >> accountNo >> parameter;

	st_SERVER_SESSION *pSession = this->FindSession(clientID);

	if (nullptr != pSession)
	{
		this->_pLoginServer->PacketProc_NewClientLogin(parameter, pSession->_serverType);
	}
	else
	{
		SYSLOG(L"", LOG::LEVEL_ERROR, L"");
		return;
	}

	//this->_pLoginServer->Response

	return;
}

unsigned __stdcall CLanServer_Login::TimeoutCheckThreadFunc(void *lpParam)
{
	CLanServer_Login *ptr = (CLanServer_Login *)lpParam;
	return ptr->TimeoutCheck_update();
}

bool CLanServer_Login::TimeoutCheck_update(void)
{
	__int64 updateTick = time(NULL);
	__int64 currTick = 0;

	while (true)
	{
		// 종료로직도 추가?

		currTick = time(NULL);
		if (currTick - currTick > dfUPDATE_TICK)
		{
			AcquireSRWLockShared(&this->_sessionMapLock);

			for (auto iter = this->_sessionMap.begin(); iter != this->_sessionMap.end(); ++iter)
			{
				if (currTick - iter->second->_timeoutTick > dfSERVER_TIMEOUT_TICK)
				{
					ClientDisconnect(iter->second->_clientID);
					SYSLOG(L"ERROR", LOG::LEVEL_ERROR, L"\'%s\' Server disconnected : timeout", iter->second->_serverName);
				}
			}
			
			ReleaseSRWLockShared(&this->_sessionMapLock);
			updateTick = time(NULL);
		}
		else
			Sleep(50);
	}

	return true;
}