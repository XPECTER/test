#include "stdafx.h"
#include "CommonProtocol.h"
#include "LoginServer.h"
#include "LanServer_Login.h"



CLoginServer::CLoginServer()
{
	memset(this->_serverLinkInfo, 0, sizeof(this->_serverLinkInfo));
	InitializeSRWLock(&this->_srwLock);

	this->_lanserver_Login = new CLanServer_Login(this);
	this->SetConfigData();

	_hMonitorTPSThread = INVALID_HANDLE_VALUE;
	_hUpdateThread = INVALID_HANDLE_VALUE;
}

CLoginServer::~CLoginServer()
{

}

bool CLoginServer::Start(void)
{
	if (!this->_lanserver_Login->Start(this->_szLanBindIP, this->_iLanBindPort, 2, false, 100))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer Login start failed.");
		return false;
	}

	if (!CNetServer::Start(this->_szBindIP, this->_iBindPort, this->_iWorkerThreadNum, this->_bNagleOpt, this->_iClientMax))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"NetServer Login start failed.");
		return false;
	}

	return true;
}

bool CLoginServer::SetConfigData()
{
	CConfigParser parser;
	wchar_t szBlock[32] = { 0, };
	wchar_t szKey[32] = { 0, };

	if (!parser.OpenConfigFile(L"LoginServerConfig.ini"))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Config file open failed");
		return false;
	}
	else
	{
		wsprintf(szBlock, L"NETWORK");
		if (!parser.MoveConfigBlock(szBlock))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s BLOCK in Config file", szBlock);
			return false;
		}
		else
		{
			wsprintf(szKey, L"SERVER_NAME");
			if (!parser.GetValue(szKey, this->_szServerName, 64))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"BIND_IP");
			if (!parser.GetValue(szKey, this->_szBindIP, 16))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"BIND_PORT");
			if (!parser.GetValue(szKey, &this->_iBindPort))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"LAN_BIND_IP");
			if (!parser.GetValue(szKey, this->_szLanBindIP, 16))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"LAN_BIND_PORT");
			if (!parser.GetValue(szKey, &this->_iLanBindPort))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"MONITORING_SERVER_IP");
			if (!parser.GetValue(szKey, this->_szMonitoringServerIP, 16))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"MONITORING_SERVER_PORT");
			if (!parser.GetValue(szKey, &this->_iMonitoringServerPort))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"WORKER_THREAD");
			if (!parser.GetValue(szKey, &this->_iWorkerThreadNum))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"NAGLE_OPT");
			if (!parser.GetValue(szKey, &this->_bNagleOpt))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}
		}

		wsprintf(szBlock, L"SYSTEM");
		if (!parser.MoveConfigBlock(szBlock))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s BLOCK in Config file", szBlock);
			return false;
		}
		else
		{
			wsprintf(szKey, L"CLIENT_MAX");
			if (!parser.GetValue(szKey, &this->_iClientMax))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"PACKET_CODE");
			if (!parser.GetValue(szKey, &this->_iPacketCode))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"PACKET_KEY1");
			if (!parser.GetValue(szKey, &this->_iPacketKey1))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"PACKET_KEY2");
			if (!parser.GetValue(szKey, &this->_iPacketKey2))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"LOG_LEVEL");
			if (!parser.GetValue(szKey, &this->_iLogLevel))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"SERVER_LINK_CNF");
			if (!parser.GetValue(szKey, this->_szServerLinkConfigFileName, 128))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			LOG::iLogLevel = this->_iLogLevel;
			CPacket::SetEncryptCode((BYTE)this->_iPacketCode, (BYTE)this->_iPacketKey1, (BYTE)this->_iPacketKey2);
		}

		wsprintf(szBlock, L"DATABASE");
		if (!parser.MoveConfigBlock(szBlock))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s BLOCK in Config file", szBlock);
			return false;
		}
		else
		{
			wsprintf(szKey, L"ACCOUNT_IP");
			if (!parser.GetValue(szKey, this->_szAccountIP, 16))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"ACCOUNT_PORT");
			if (!parser.GetValue(szKey, &this->_iAccountPort))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"ACCOUNT_USER");
			if (!parser.GetValue(szKey, this->_szAccountUser, 32))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"ACCOUNT_PASSWORD");
			if (!parser.GetValue(szKey, this->_szAccountPassword, 32))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"ACCOUNT_DBNAME");
			if (!parser.GetValue(szKey, this->_szAccountDBName, 32))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"LOG_IP");
			if (!parser.GetValue(szKey, this->_szLogIP, 16))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"LOG_PORT");
			if (!parser.GetValue(szKey, &this->_iLogPort))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"LOG_USER");
			if (!parser.GetValue(szKey, this->_szLogUser, 32))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"LOG_PASSWORD");
			if (!parser.GetValue(szKey, this->_szLogPassword, 32))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}

			wsprintf(szKey, L"LOG_DBNAME");
			if (!parser.GetValue(szKey, this->_szLogDBName, 32))
			{
				SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"Not exist %s Key", szKey);
				return false;
			}
		}
	}
	
	if (!parser.OpenConfigFile(L"ServerLink.ini"))
	{
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer_Login ConfigFile Open Failed");
		return false;
	}
	else
	{
		wsprintf(szBlock, L"SERVER_LINK");
		if (!parser.MoveConfigBlock(szBlock))
		{
			SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"LanServer_Login not exist %s block", szBlock);
			return false;
		}
		else
		{
			for (int i = 0; i < dfSERVER_LINK_NUM; ++i)
			{
				wsprintf(szKey, L"%d_NAME", i + 1);
				if (!parser.GetValue(szKey, this->_serverLinkInfo[i]._serverName, dfSERVER_NAME_LEN))
					break;

				wsprintf(szKey, L"%d_GAMEIP", i + 1);
				if (!parser.GetValue(szKey, this->_serverLinkInfo[i]._gameServerIP, dfSERVER_NAME_LEN))
				{
					SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[LanServer_Login] not exist %s key", szKey);
					return false;
				}

				wsprintf(szKey, L"%d_GAMEPORT", i + 1);
				if (!parser.GetValue(szKey, &this->_serverLinkInfo[i]._gameServerPort))
				{
					SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[LanServer_Login] not exist %s key", szKey);
					return false;
				}

				wsprintf(szKey, L"%d_CHATIP", i + 1);
				if (!parser.GetValue(szKey, this->_serverLinkInfo[i]._chatServerIP, dfSERVER_NAME_LEN))
				{
					SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[LanServer_Login] not exist %s key", szKey);
					return false;
				}

				wsprintf(szKey, L"%d_CHATPORT", i + 1);
				if (!parser.GetValue(szKey, &this->_serverLinkInfo[i]._chatServerPort))
				{
					SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[LanServer_Login] not exist %s key", szKey);
					return false;
				}

				wsprintf(szKey, L"%d_GAMEIP_DUMMY", i + 1);
				if (!parser.GetValue(szKey, this->_serverLinkInfo[i]._gameServerIP_Dummy, dfSERVER_NAME_LEN))
				{
					SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[LanServer_Login] not exist %s key", szKey);
					return false;
				}

				wsprintf(szKey, L"%d_GAMEPORT_DUMMY", i + 1);
				if (!parser.GetValue(szKey, &this->_serverLinkInfo[i]._gameServerPort_Dummy))
				{
					SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[LanServer_Login] not exist %s key", szKey);
					return false;
				}

				wsprintf(szKey, L"%d_CHATIP_DUMMY", i + 1);
				if (!parser.GetValue(szKey, this->_serverLinkInfo[i]._chatServerIP_Dummy, dfSERVER_NAME_LEN))
				{
					SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[LanServer_Login] not exist %s key", szKey);
					return false;
				}

				wsprintf(szKey, L"%d_CHATPORT_DUMMY", i + 1);
				if (!parser.GetValue(szKey, &this->_serverLinkInfo[i]._chatServerPort_Dummy))
				{
					SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[LanServer_Login] not exist %s key", szKey);
					return false;
				}
			}
		}
	}
	return true;
}

bool CLoginServer::OnConnectionRequest(wchar_t *szClientIP, int iPort)
{
	return true;
}

void CLoginServer::OnClientJoin(CLIENT_ID clientID)
{
	this->InsertPlayer(clientID);
	InterlockedIncrement(&this->_Monitor_LoginWait);
	return;
}

void CLoginServer::OnClientLeave(CLIENT_ID clientID)
{
	this->RemovePlayer(clientID);
	return;
}

void CLoginServer::OnRecv(CLIENT_ID clientID, CPacket *pPacket)
{
	WORD type;

	*pPacket >> type;

	switch (type)
	{
		case en_PACKET_CS_LOGIN_REQ_LOGIN:
			this->PacketProc_ReqLogin(clientID, pPacket);
			break;

		default:
			break;
	}
}

void CLoginServer::OnSend(CLIENT_ID clientID, int sendsize)
{
	this->ClientDisconnect(clientID);
	return;
}

void CLoginServer::OnWorkerThreadBegin(void)
{
	return;
}

void CLoginServer::OnWorkerThreadEnd(void)
{
	return;
}

void CLoginServer::OnError(int errorNo, CLIENT_ID clientID, WCHAR *errstr)
{
	return;
}

// 서버 타입과 이름을 받아서 등록하는 함수
int CLoginServer::CheckServerLink(wchar_t *serverName)
{
	for (int i = 0; i < dfSERVER_LINK_NUM; ++i)
	{
		if (0 == wcscmp(this->_serverLinkInfo[i]._serverName, serverName))
		{
			return ++i;
		}
	}
	
	return 0;
}

bool CLoginServer::InsertPlayer(CLIENT_ID clientID)
{
	st_PLAYER *pPlayer = this->_playerPool.Alloc();
	pPlayer->_clientID = clientID;
	pPlayer->_timeoutTick = time(NULL);
	pPlayer->_bChatServerRecv = false;
	pPlayer->_bGameServerRecv = false;

	AcquireSRWLockExclusive(&this->_srwLock);
	this->_playerMap.insert(std::pair<CLIENT_ID, st_PLAYER *>(clientID, pPlayer));
	ReleaseSRWLockExclusive(&this->_srwLock);

	return true;
}

bool CLoginServer::RemovePlayer(CLIENT_ID clientID)
{
	AcquireSRWLockExclusive(&this->_srwLock);

	auto iter = this->_playerMap.find(clientID);
	if (iter == this->_playerMap.end())
	{
		ReleaseSRWLockExclusive(&this->_srwLock);
		SYSLOG(L"SYSTEM", LOG::LEVEL_ERROR, L"[CLIENT_ID : %d] not exist player in map", EXTRACTCLIENTID(clientID));
		return false;
	}

	this->_playerPool.Free(iter->second);
	this->_playerMap.erase(iter);

	ReleaseSRWLockExclusive(&this->_srwLock);
	return true;
}

CLoginServer::st_PLAYER* CLoginServer::FindPlayer(CLIENT_ID clientID)
{
	st_PLAYER *pPlayer = nullptr;
	
	AcquireSRWLockShared(&this->_srwLock);

	auto iter = this->_playerMap.find(clientID);
	if (iter != this->_playerMap.end())
		pPlayer = iter->second;
	
	ReleaseSRWLockShared(&this->_srwLock);
	
	return pPlayer;
}

//CLoginServer::st_PLAYER* CLoginServer::FindPlayer(__int64 accountNo)
//{
//	st_PLAYER *pPlayer = nullptr;
//
//	AcquireSRWLockShared(&this->_srwLock);
//	for (auto iter = this->_playerMap.begin(); iter != this->_playerMap.end(); ++iter)
//	{
//		if (accountNo == iter->second->_accountNo)
//		{
//			pPlayer = iter->second;
//			break;
//		}
//	}
//	
//	ReleaseSRWLockShared(&this->_srwLock);
//	return pPlayer;
//}

bool CLoginServer::PacketProc_ReqLogin(CLIENT_ID clientID, CPacket *pPacket)
{
	__int64 accountNo;
	char SessionKey[64] = { 0, };

	*pPacket >> accountNo;
	pPacket->Dequeue(SessionKey, 64);

	// 내가 Select한 SessionKey와 유저가 가져온 SessionKey가 다르면 컷
	//if (accountNo > dfDUMMY_ACCOUNTNO_MAX && SessionKey != DB에서 찾은 세션 키)
	//{
	//	SendPacket_LoginFailed()
	//	return false;
	//}

	CPacket *pSendPacket = CPacket::Alloc();
	pSendPacket->SetHeaderSize(2);

	*pSendPacket << (WORD)en_PACKET_SS_REQ_NEW_CLIENT_LOGIN;
	*pSendPacket << accountNo;
	pSendPacket->Enqueue(SessionKey, 64);
	*pSendPacket << clientID;

	HEADER header;
	header.wSize = pSendPacket->GetPayloadUseSize();
	pSendPacket->InputHeader((char *)&header, sizeof(HEADER));

	this->_lanserver_Login->SendPacket_ServerGroup(1, pSendPacket);
	pSendPacket->Free();

	return true;
}

bool CLoginServer::PacketProc_NewClientLogin(CLIENT_ID clientID, int serverType)
{
	st_PLAYER *pPlayer = FindPlayer(clientID);

	if (nullptr != pPlayer)
	{
		switch (serverType)
		{
			case dfSERVER_TYPE_GAME:
				pPlayer->_bGameServerRecv = true;
				break;

			case dfSERVER_TYPE_CHAT:
				pPlayer->_bChatServerRecv = true;
				break;
				
			default:
				SYSLOG(L"PACKET", LOG::LEVEL_DEBUG, L"wrong servertype recv");
				return false;
		}

		if (true == pPlayer->_bGameServerRecv && true == pPlayer->_bChatServerRecv)
			this->SendPacket_ResponseLogin(pPlayer->_clientID, dfMONITOR_TOOL_LOGIN_OK);

		return true;
	}
	else
	{
		SYSLOG(L"ERROR", LOG::LEVEL_ERROR, L"Found Player is not exist");
		return false;
	}
}

// 서버군이 1개다라는 전제가 있는거 같음
// 더미면 세션키 막 생성해서 보내야해?
// 로그인 실패일때는 나머지를 넣어줘야 하는가?

void CLoginServer::SendPacket_ResponseLogin(CLIENT_ID clientID, BYTE state)
{
	st_PLAYER *pPlayer = FindPlayer(clientID);

	CPacket *pPacket = CPacket::Alloc();
	*pPacket << (WORD)en_PACKET_CS_LOGIN_RES_LOGIN;
	*pPacket << pPlayer->_accountNo;
	*pPacket << state;

	// 로그인 실패일때는 나머지를 넣어줘야 하는가?

	// 여기에 DB가 들어가야함.
	// ID넣고
	// Nickname 넣고

	if (pPlayer->_accountNo > dfDUMMY_ACCOUNTNO_MAX)
	{
		pPacket->Enqueue((char *)this->_serverLinkInfo[0]._gameServerIP, 32);
		*pPacket << (WORD)this->_serverLinkInfo[0]._gameServerPort;
		pPacket->Enqueue((char *)this->_serverLinkInfo[0]._chatServerIP, 32);
		*pPacket << (WORD)this->_serverLinkInfo[0]._chatServerPort;
	}
	else
	{
		pPacket->Enqueue((char *)this->_serverLinkInfo[0]._gameServerIP_Dummy, 32);
		*pPacket << (WORD)this->_serverLinkInfo[0]._gameServerPort_Dummy;
		pPacket->Enqueue((char *)this->_serverLinkInfo[0]._chatServerIP_Dummy, 32);
		*pPacket << (WORD)this->_serverLinkInfo[0]._chatServerPort_Dummy;
	}

	HEADER header;
	header.wSize = pPacket->GetPayloadUseSize();
	pPacket->InputHeader((char *)&header, sizeof(HEADER));

	SendPacket(pPlayer->_clientID, pPacket);
	pPacket->Free();
}

unsigned __stdcall CLoginServer::UpdateThreadFunc(void *pParam)
{
	CLoginServer *pServer = (CLoginServer *)pParam;
	return pServer->UpdateThread_update();
}

bool CLoginServer::UpdateThread_update(void)
{
	__int64 updateTick = time(NULL);
	__int64 currTick = 0;

	while (true)
	{
		currTick = time(NULL);

		if (currTick - updateTick > dfUPDATE_TICK)
		{
			AcquireSRWLockShared(&_srwLock);

			for (auto iter = this->_playerMap.begin(); iter != this->_playerMap.end(); ++iter)
			{
				if (time(NULL) - iter->second->_timeoutTick > dfPLAYER_TIMEOUT_TICK)
				{
					ClientDisconnect(iter->second->_clientID);
					SYSLOG(L"PLAYER", LOG::LEVEL_DEBUG, L"[%d]Player disconnected : timeout", iter->second->_accountNo);
				}
			}

			ReleaseSRWLockShared(&_srwLock);
			updateTick = time(NULL);
		}
		else
		{
			Sleep(50);
		}
	}

	return true;
}